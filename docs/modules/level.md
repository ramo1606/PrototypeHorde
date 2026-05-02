# level

A level is a struct of function pointers. The active level fully owns its
stage of the game's lifecycle (init, input, update, render). Levels are
declared as static globals (`extern Level Level_X;`) and never
heap-allocated — switching levels is just a pointer swap.

This module is small enough to live in a single header (`level.h`); there
is no implementation file.

## Types

- `Level` — the level vtable. All callbacks receive `Game*` so the level
  can access subsystems (renderer, physics, arenas, etc.).
- Function pointer typedefs:
  - `LevelInitFn(game)` — set up assets, register entities, allocate from
    the level arena.
  - `LevelShutdownFn(game)` — release GPU resources (raylib unloads).
    Memory in the level arena is freed implicitly by the manager.
  - `LevelInputFn(game)` — read input. Called on the fixed tick.
  - `LevelUpdateFn(game, dt)` — gameplay logic. Called on the fixed tick.
  - `LevelRender3DFn(game, alpha)` — 3D pass, runs inside `BeginMode3D`.
    `alpha` is the interpolation factor.
  - `LevelRenderHUDFn(game, alpha)` — 2D pass, runs after `EndMode3D`.

## Defining a level

```c
/* level_gameplay.c */
static void Init(Game* game)         { ... }
static void Shutdown(Game* game)     { ... }
static void ProcessInput(Game* game) { ... }
static void Update(Game* game, float dt) { ... }
static void Render3D(Game* game, float alpha) { ... }
static void RenderHUD(Game* game, float alpha) { ... }

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

Any field may be `NULL`; the level manager skips unregistered callbacks.

## Why function pointers (not enums + switch)

Each level lives in its own `.c` file with file-scoped state and helpers,
no leakage. Adding a new level is a new file plus one `extern` declaration.
There is no central `switch (levelID)` to grow over time. The dispatch cost
(one indirect call per phase) is negligible at the scale of "a few times
per frame."

## Lifetime contract

- `Init` allocates from `game->level` (the level arena). Anything allocated
  here is gone after the next `LevelManagerTransitionTo` swap.
- `Shutdown` must release GPU/system resources (raylib models, sounds)
  because the arena reset only frees RAM. See `arena.md`.
- The level should never directly free arena memory; the manager handles
  the bulk reset.
