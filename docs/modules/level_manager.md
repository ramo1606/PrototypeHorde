# level_manager (kit)

Owns the active `Level*` (vtable of function pointers) and runs a small
state machine to swap between levels with a fullscreen visual effect.
The swap (Shutdown old, optional `onSwap` cleanup, Init new) happens at
the apex of the fade-out, so the player never sees the teardown frame.

The module is host-agnostic: every level callback receives a `void*
user` set at init. The host casts to whatever struct it owns
(`Game`, `App`, `Editor`).

Lives under `lib/` as part of the reusable kit.

## Dependencies & overrides

```
DEPENDENCIES: raylib.h
OVERRIDES: none
```

## Types

### Level vtable

```c
typedef void (*LevelInitFn)(void* user);
typedef void (*LevelShutdownFn)(void* user);
typedef void (*LevelInputFn)(void* user);
typedef void (*LevelUpdateFn)(void* user, float dt);
typedef void (*LevelRender3DFn)(void* user, float alpha);
typedef void (*LevelRenderHUDFn)(void* user, float alpha);

typedef struct Level {
    const char* name;
    LevelInitFn      Init;
    LevelShutdownFn  Shutdown;
    LevelInputFn     ProcessInput;
    LevelUpdateFn    Update;
    LevelRender3DFn  Render3D;
    LevelRenderHUDFn RenderHUD;
} Level;
```

Any field may be NULL; the manager skips unregistered callbacks.

### Manager

- `TransitionState` — `IDLE` / `FADING_OUT` / `FADING_IN`.
- `TransitionEffectFn(progress)` — fullscreen overlay function.
  `progress` is `[0, 1]`: 0 invisible, 1 fully covered.
- `LevelSwapFn(user)` — optional cleanup at apex of fade-out.
- `LevelManager` — active level, pending level, state, effects,
  duration, progress, `void* user`, `LevelSwapFn onSwap`.

## Public API

### Lifecycle

| Function | Purpose |
|---|---|
| `LevelManagerInit(*mgr, user, initialLevel)` | Set up state, store `user`, call the initial level's `Init` directly (no transition). |
| `LevelManagerShutdown(*mgr)` | Call the active level's `Shutdown(user)`. |
| `LevelManagerUpdate(*mgr, dt)` | Advance the transition state machine. Once per visual frame. |
| `LevelManagerRender(*mgr)` | Draw the transition overlay if a transition is in progress. Call last so it covers everything. |

### Transition control

| Function | Purpose |
|---|---|
| `LevelManagerTransitionTo(*mgr, level, effectOut, effectIn, duration)` | Start a transition with custom effects and duration. |
| `LevelManagerSwitchTo(*mgr, level)` | Convenience wrapper using the default fade. |

### Queries

| Function | Purpose |
|---|---|
| `LevelManagerIsTransitioning(*mgr)` | True while not `IDLE`. |
| `LevelManagerGetActiveLevel(*mgr)` | Current `Level*`. |
| `LevelManagerGetStateName(*mgr)` | Human-readable state, for debug. |
| `LevelManagerGetProgress(*mgr)` | Current 0..1 progress, for debug. |

### Level callback delegation

| Function | Purpose |
|---|---|
| `LevelManagerProcessInput(*mgr)` | Forward to active level's `ProcessInput(user)`. |
| `LevelManagerUpdateLevel(*mgr, dt)` | Forward to active level's `Update(user, dt)`. |
| `LevelManagerRender3D(*mgr, alpha)` | Forward to active level's `Render3D(user, alpha)`. |
| `LevelManagerRenderHUD(*mgr, alpha)` | Forward to active level's `RenderHUD(user, alpha)`. |

These wrappers exist so the host doesn't have to null-check or cast on
every call site, and so future hooks (pause-on-transition, input
blocking, stat collection) fit without touching every caller.

## Built-in transition effects

| Function | Effect |
|---|---|
| `TransitionFade(progress)` | Black overlay, alpha = `progress * 255`. |
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
                           applySwap()  (Shutdown + onSwap + Init)
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

Internal helper called at the apex of fade-out (screen fully covered):

1. Old level's `Shutdown(user)` — releases GPU resources.
2. `onSwap(user)` — host cleanup hook (typically resets a memory
   arena). May be NULL.
3. `activeLevel = pendingLevel`.
4. New level's `Init(user)` — sets up state in the freshly cleared
   arena.

Doing the swap during full coverage means the player never sees a
frame with half-loaded assets or a black scene.

## Defining a level (host-side)

```c
/* level_gameplay.c */
static void Init(void* user) {
    Game* game = (Game*)user;
    /* allocate level data, register entities, ... */
}
static void Update(void* user, float dt) { ... }
static void Render3D(void* user, float alpha) { ... }
/* etc */

Level LEVEL_GAMEPLAY = {
    .name         = "Gameplay",
    .Init         = Init,
    .Shutdown     = Shutdown,
    .ProcessInput = ProcessInput,
    .Update       = Update,
    .Render3D     = Render3D,
    .RenderHUD    = RenderHUD,
};
```

## Wiring (host-side)

```c
LevelManagerInit(&mgr, game, &LEVEL_FIRST);
mgr.onSwap = onLevelSwap;   /* e.g. ArenaReset(&game->level) */

/* per frame */
LevelManagerUpdate(&mgr, frameTime);
/* per tick */
LevelManagerProcessInput(&mgr);
LevelManagerUpdateLevel(&mgr, FIXED_TIMESTEP);
/* per render */
LevelManagerRender3D(&mgr, alpha);
LevelManagerRenderHUD(&mgr, alpha);
LevelManagerRender(&mgr);  /* transition overlay last */
```

## Guardrails

- `TransitionTo` with `level == NULL` logs a warning and is a no-op.
- `TransitionTo` while already transitioning is rejected (logged).
- `TransitionTo` with `level == activeLevel` is rejected (logged).
- `duration <= 0` falls back to `TRANSITION_DEFAULT_DURATION`.
