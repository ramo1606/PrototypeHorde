# game

Top-level orchestrator. Owns the three memory arenas, every subsystem
(level manager, renderer, physics, camera) by value, and runs the main
game loop with fixed-timestep updates and decoupled rendering.

## Types

- `Game` — the whole runtime state. Heap-allocated in `main()` (~50 KB+,
  too large for the stack). Holds the three arenas, the four subsystem
  structs by value, the timing state, the frametime stats, and a
  `running` flag.

## Public API

| Function | Purpose |
|---|---|
| `GameInit(*game, *initialLevel)` | Bring up the window, audio, and every subsystem in order. Initialize the first level directly (no transition). Returns false on failure. |
| `GameShutdown(*game)` | Tear down in reverse: shut down the active level, then renderer, debug, resource manager; close audio and window; destroy the three arenas. |
| `GameRun(*game)` | Run the main loop until `WindowShouldClose()` or `running == false`. |

## Bring-up order

```
ArenaCreate × 3 (permanent, level, scratch)
InitWindow + InitAudioDevice
ResourceInit
DebugInit
RendererInit
CameraInit
PhysicsInit
LevelManagerInit  (calls initialLevel->Init)
DebugRegister3D(2, physDebugDraw3D)   // F4 panel hooks physics debug draw
```

Tear-down in `GameShutdown` is the reverse, with the level manager going
first so the active level's `Shutdown` runs before the subsystems it
depended on are gone.

## Main loop

```
while (!WindowShouldClose() && game->running) {
    frameTime = clamp(GetFrameTime(), 0, MAX_DELTA_TIME)
    updateFrametimeStats(frameTime)
    LevelManagerUpdate(...)               // transition state machine

    accumulator += frameTime
    while (accumulator >= FIXED_TIMESTEP) {
        RendererPreUpdate()               // curr → prev
        LevelManagerProcessInput(...)
        LevelManagerUpdateLevel(..., FIXED_TIMESTEP)
        PhysicsUpdate(..., NULL, NULL)    // triggers + passive pairs
        accumulator -= FIXED_TIMESTEP
        if (++updateCount >= MAX_UPDATES_PER_FRAME) { drop remainder; break }
    }
    alpha = accumulator / FIXED_TIMESTEP
    ArenaReset(scratch)
    CameraUpdate(frameTime)
    RendererSetCamera(camera.camera)

    BeginDrawing()
        ClearBackground(...)
        RendererBuildDrawList(alpha)
        BeginMode3D(camera)
            RendererDraw3D()
            LevelManagerRender3D(alpha)
            DebugRender3D()
        EndMode3D()
        LevelManagerRenderHUD(alpha)
        DebugUpdate() / feedDebugStats() / DebugRender()
        LevelManagerRender()              // transition overlay last
    EndDrawing()
}
```

### Spiral-of-death protection

- `MAX_DELTA_TIME` clamps a single frametime so a stall (debugger pause,
  OS hiccup) doesn't dump 30 seconds of physics into the accumulator.
- `MAX_UPDATES_PER_FRAME` caps fixed steps per visual frame; on overrun,
  the remaining accumulator is dropped instead of letting the loop fall
  further behind.

### Why camera updates per visual frame, not per tick

The level sets the camera target during its fixed-tick `Update`. The
camera smooths toward that target using the visual `frameTime`, so the
camera moves smoothly even when the visual rate is 30 Hz against a
60 Hz fixed tick. The camera's `Camera3D` is then pushed to the renderer
**before** `BuildDrawList` so culling and the BeginMode3D context use the
fresh camera.

### Scratch arena reset

`ArenaReset(&game->scratch)` happens once per visual frame, after all
fixed ticks. Anything allocated from `scratch` is gone next frame.
Per-tick gameplay code can rely on it for short-lived buffers (overlap
results, AOE lists) without bookkeeping.

## Frametime stats

The accumulator (min / max / avg over a 1-second window) feeds the F2
performance panel via `feedDebugStats`. The window resets every second
so the displayed min/max reflect recent behavior, not the all-time worst
since launch.

## Debug callback wiring

`physDebugDraw3D(Game*)` is a tiny wrapper static in `game.c` that
forwards to `PhysicsDebugDraw(&game->physWorld)`. The wrapper exists
because the debug system's `DebugRender3DFn` signature takes a `Game*`,
not a `PhysWorld*` — the wrapper picks the right field. Same pattern
will repeat for any subsystem that wants its own F-key panel.

## What lives here vs. in subsystems

`game.c` only contains the **director** logic: subsystem ordering, the
loop, timing math, the stats fan-out. Anything substantive about a
subsystem (registration, queries, drawing) lives in that subsystem.
Adding gameplay never means editing `game.c`.

The exception is the per-frame `feedDebugStats` aggregator, which
necessarily reaches into every subsystem's struct to assemble the
`DebugPerfStats`. That coupling is one-way (game reads, subsystems
don't know about debug) and centralized in one function.
