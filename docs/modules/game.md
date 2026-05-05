# game (project)

Top-level orchestrator for Prototype Horde. Owns the three memory arenas,
every kit subsystem (renderer, physics, level manager) by value, the
camera, and the project-wide render state that sits above the kit.
Runs the main loop with fixed-timestep updates and decoupled rendering.

This is **project code**, not part of the kit. It pulls kit modules
together and provides the `Game` type.

## Types

- `Game` — the whole runtime state. Heap-allocated in `main()` (~50 KB+,
  too large for the stack).

## Public API

| Function | Purpose |
|---|---|
| `GameInit(*game, *initialLevel)` | Bring up window, audio, every subsystem, and the first level. Returns false on fatal setup failure. |
| `GameShutdown(*game)` | Tear down in reverse: shut down active level, subsystems, audio/window, destroy arenas. |
| `GameRun(*game)` | Main loop until `WindowShouldClose()` or `running == false`. |

### Render state helper

| Function | Purpose |
|---|---|
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
LevelManagerInit (calls initialLevel->Init)
levelMgr.onSwap = onLevelSwap
DebugRegister3D(2, physDebugDraw3D)
```

Each step that can fail is gated. Fatal failures (arenas, window) call
`gameInitCleanup` which tears down everything that did succeed before
returning false. Audio init is non-fatal: the game logs a warning and
continues silent.

Tear-down in `GameShutdown` is the reverse, with the level manager
going first so the active level's `Shutdown` runs before the
subsystems it depends on are gone.

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
            RendererDraw3D(&renderer)
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

## Project-owned render state

The kit renderer is intentionally minimal. The project currently owns:

- **Clear color** — `game->clearColor` passed to `ClearBackground`.

Stylized shading, blob shadows, and similar polish passes are outside
the kit and can be added later without changing `renderer.c`.

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
- Project-specific render state (`clearColor`).
- The debug stats fan-out (one-way: game reads, kit modules don't know
  about debug).

Anything substantive about a kit subsystem (registration, queries,
drawing) lives in that subsystem.
