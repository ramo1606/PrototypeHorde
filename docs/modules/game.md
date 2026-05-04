# game (project)

Top-level orchestrator for Boxhead 3D. Owns the three memory arenas,
every kit subsystem (renderer, physics, level manager) by value, the
camera, and the host-side render glue (cel shader + blob shadows).
Runs the main loop with fixed-timestep updates and decoupled rendering.

This is **project code**, not part of the kit. It pulls kit modules
together and provides the `Game` type.

## Types

- `Game` — the whole runtime state. Heap-allocated in `main()` (~50 KB+,
  too large for the stack).

## Public API

| Function | Purpose |
|---|---|
| `GameInit(*game, *initialLevel)` | Bring up window, audio, every subsystem; load cel shader and blob shadow resources; init the first level. Returns false on failure. |
| `GameShutdown(*game)` | Tear down in reverse: shut down active level, unload effect resources, kit subsystems, audio/window, destroy arenas. |
| `GameRun(*game)` | Main loop until `WindowShouldClose()` or `running == false`. |

### Render glue (host-side helpers)

| Function | Purpose |
|---|---|
| `GameApplyDefaultShader(*game, *model)` | Assign the project's cel shader to every material of `model`. Call before `RendererRegister`. |
| `GameSetBlobShadow(*game, handle, on, radius)` | Toggle a blob shadow under a registered renderable. |
| `GameSetClearColor(*game, color)` | Background color used each frame. |

## Bring-up order

```
ArenaCreate × 3 (permanent, level, scratch)   ── may fail → cleanup → return false
InitWindow                                    ── may fail → cleanup → return false
InitAudioDevice                               ── may fail → log + continue (silent)
clearColor = default
DebugInit
RendererInit
CameraInit
PhysicsInit
loadCelShader(game)         /* may fail → log + continue (default shader) */
loadShadowResources(game)   /* may fail → log + continue (no blob shadows) */
LevelManagerInit (calls initialLevel->Init)
levelMgr.onSwap = onLevelSwap
DebugRegister3D(2, physDebugDraw3D)
```

Each step that can fail is gated. Fatal failures (arenas, window) call
`gameInitCleanup` which tears down everything that did succeed before
returning false. Non-fatal failures (audio, shaders, shadow texture)
log a warning and continue with degraded behavior — `applyCelShaderUniforms`,
`drawBlobShadows`, and `GameApplyDefaultShader` early-return when their
resources didn't load.

Tear-down in `GameShutdown` is the reverse, with the level manager
going first so the active level's `Shutdown` runs before the
subsystems it depends on are gone. Each `Unload*` is guarded by an
`id != 0` check so partial-init shutdown is safe.

## Main loop

```
while (!WindowShouldClose() && game->running) {
    frameTime = clamp(GetFrameTime(), 0, MAX_DELTA_TIME)
    updateFrametimeStats(frameTime)
    LevelManagerUpdate(&mgr, frameTime)            /* transition state machine */

    accumulator += frameTime
    while (accumulator >= FIXED_TIMESTEP) {
        RendererPreUpdate()                         /* curr → prev */
        LevelManagerProcessInput(&mgr)
        LevelManagerUpdateLevel(&mgr, FIXED_TIMESTEP)
        PhysicsUpdate(NULL, NULL)
        accumulator -= FIXED_TIMESTEP
        if (++updateCount >= MAX_UPDATES_PER_FRAME) { drop remainder; break }
    }
    alpha = accumulator / FIXED_TIMESTEP
    ArenaReset(&scratch)
    CameraUpdate(frameTime)

    BeginDrawing();
        ClearBackground(clearColor)
        RendererBuildDrawList(&renderer, camera.camera, alpha)
        BeginMode3D(camera.camera)
            applyCelShaderUniforms(game)            /* host glue */
            RendererDraw3D(&renderer)
            drawBlobShadows(game)                   /* host glue */
            LevelManagerRender3D(&mgr, alpha)
            DebugRender3D(game)
        EndMode3D()
        LevelManagerRenderHUD(&mgr, alpha)
        DebugUpdate / feedDebugStats / DebugRender
        LevelManagerRender(&mgr)                    /* transition overlay last */
    EndDrawing();
}
```

### Spiral-of-death protection

- `MAX_DELTA_TIME` clamps a single frametime so a stall doesn't dump
  30 seconds of physics into the accumulator.
- `MAX_UPDATES_PER_FRAME` caps fixed steps per visual frame; on
  overrun, the remaining accumulator is dropped.

### Camera per visual frame, not per tick

The level sets the camera target during its fixed-tick `Update`. The
camera smooths toward that target using the visual `frameTime`. The
fresh `Camera3D` is then passed to `RendererBuildDrawList` and
`BeginMode3D`.

### Scratch arena reset

`ArenaReset(&game->scratch)` happens once per visual frame, after all
fixed ticks. Per-tick gameplay code can rely on it for short-lived
buffers without bookkeeping.

## Host-side render glue

The kit renderer is intentionally minimal. The project owns:

- **Cel shader** — `loadCelShader` calls raylib `LoadShader` directly.
  Uniform locations cached. `applyCelShaderUniforms` pushes
  `lightDir`, `ambient`, `numBands` once per frame inside `BeginMode3D`.
  Models get the shader assigned via `GameApplyDefaultShader` before
  registration.
- **Blob shadows** — `loadShadowResources` generates a 64×64 radial
  gradient texture and a unit plane. `drawBlobShadows` walks
  `renderer.drawList` (post-cull, interpolated) and draws a textured
  plane under any entity tagged in `game->blobOn[]`.
- **Clear color** — `game->clearColor` passed to `ClearBackground`.

If you replace the cel shader with another (Phong, PBR, etc.) you only
touch `loadCelShader` and `applyCelShaderUniforms`. The kit doesn't
care.

## Level swap callback

`onLevelSwap(void* user)` is set on the level manager after init. It
fires at the apex of fade-out, between the old level's `Shutdown` and
the new level's `Init`. In this project it does
`ArenaReset(&game->level)` — frees per-level allocations all at once.

## Frametime stats

Min / max / avg over a 1-second window, fed to the F2 debug panel via
`feedDebugStats`. The window resets every second so the displayed
values reflect recent behavior.

## What lives here vs. in the kit

`game.c` only contains:

- The loop director (subsystem ordering, timing).
- Project-specific render glue (cel shader, blob shadows, clear color).
- The debug stats fan-out (one-way: game reads, kit modules don't know
  about debug).

Anything substantive about a kit subsystem (registration, queries,
drawing) lives in that subsystem.
