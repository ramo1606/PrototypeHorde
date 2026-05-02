# level_manager

Owns the active `Level*` and runs a small state machine to transition
between levels with a fullscreen visual effect (fade, wipe, etc.). The
swap (Shutdown old, Reset arena, Init new) happens at the peak of the
fade-out, so the player never sees the teardown frame.

## Types

- `TransitionState` — `IDLE` / `FADING_OUT` / `FADING_IN`.
- `TransitionEffectFn(progress)` — fullscreen overlay function. `progress`
  is `[0, 1]`: 0 = invisible, 1 = fully covered.
- `LevelManager` — owns active level, pending level, current state, the
  two effect callbacks (out and in), duration, and progress.

## Public API

| Function | Purpose |
|---|---|
| `LevelManagerInit(*mgr, *game, initialLevel)` | Set up state and call the initial level's `Init` directly (no transition). |
| `LevelManagerShutdown(*mgr, *game)` | Call the active level's `Shutdown`. Used at game exit. |
| `LevelManagerUpdate(*mgr, *game, dt)` | Advance the transition state machine. Runs every visual frame. |
| `LevelManagerRender(*mgr)` | Draw the transition overlay if a transition is in progress. Call last so it covers everything. |
| `LevelManagerTransitionTo(*mgr, level, effectOut, effectIn, duration)` | Start a transition with custom effects and duration. |
| `LevelManagerSwitchTo(*mgr, level)` | Convenience wrapper using the default fade. |
| `LevelManagerIsTransitioning(*mgr)` | True while not `IDLE`. |
| `LevelManagerGetActiveLevel(*mgr)` | Current `Level*` (may be `NULL`). |
| `LevelManagerGetStateName(*mgr)` | Human-readable state, for debug. |
| `LevelManagerGetProgress(*mgr)` | Current 0..1 progress, for debug. |
| `LevelManagerProcessInput(*mgr, *game)` | Forward to active level's `ProcessInput`. |
| `LevelManagerUpdateLevel(*mgr, *game, dt)` | Forward to active level's `Update`. |
| `LevelManagerRender3D(*mgr, *game, alpha)` | Forward to active level's `Render3D`. |
| `LevelManagerRenderHUD(*mgr, *game, alpha)` | Forward to active level's `RenderHUD`. |

## Built-in transition effects

| Function | Effect |
|---|---|
| `TransitionFade(progress)` | Black overlay with alpha = `progress * 255`. |
| `TransitionWipeLeft(progress)` | Black bar grows from left to right. |
| `TransitionWipeRight(progress)` | Black bar grows from right to left. |

`effectIn = NULL` reuses `effectOut` in reverse — useful for symmetric
effects like a fade.

## State machine

```
IDLE  ──TransitionTo()──►  FADING_OUT
                                │
                                │  progress hits 1.0
                                ▼
                           applySwap()  (Shutdown + ArenaReset + Init)
                                │
                                ▼
                           FADING_IN
                                │
                                │  progress drops back to 0.0
                                ▼
                              IDLE
```

`progress` runs `0 → 1` during `FADING_OUT`, then back `1 → 0` during
`FADING_IN`. A transition effect renders correctly in both phases by
reading `progress` directly.

## The swap (`applySwap`)

Internal helper called at the apex of the fade-out (screen fully covered):

1. Call old level's `Shutdown(game)` — releases GPU resources.
2. `ArenaReset(&game->level)` — wipes all RAM allocated for the old level.
3. Set `activeLevel = pendingLevel`.
4. Call new level's `Init(game)` — sets up its state in the freshly-reset
   arena.

Doing the swap during full coverage means the player never sees a frame
with half-loaded assets or a black scene.

## Why the manager forwards level callbacks

The game loop calls `LevelManagerProcessInput`, `LevelManagerUpdateLevel`,
etc. instead of `mgr->activeLevel->ProcessInput(game)` directly. Two
reasons:

1. **Null-safety** — the manager skips callbacks the level didn't set
   without forcing every call site to null-check.
2. **Future hooks** — pause-on-transition, input-blocking during fades,
   stat collection, etc. fit naturally inside the forwarding wrappers
   without touching every caller.

## Guardrails

- `TransitionTo` with `level == NULL` logs a warning and is a no-op.
- `TransitionTo` while already transitioning is rejected (logged).
- `TransitionTo` with `level == activeLevel` is rejected (logged).
- `duration <= 0` falls back to `TRANSITION_DEFAULT_DURATION`.
