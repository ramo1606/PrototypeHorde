# BLUEPRINT — Starting a new project from this kit

The `lib/` folder is a small set of reusable C99 modules built on
raylib. They are independent of any specific game and ship as plain
`.h` + `.c` pairs (and one header-only). Drop them into a new project,
add a small amount of host glue, and you have the skeleton of a game.

## What's in `lib/`

| Module | Files | What it does |
|---|---|---|
| arena | `arena.h` | Header-only wrapper over rmem's `MemPool`. Bump allocators with freelist. |
| renderer | `renderer.h`, `renderer.c` | Pool of registered models with interpolation, frustum culling, material sorting, draw list. |
| physics | `physics.h`, `physics.c` | AABB / sphere / capsule colliders, layer-mask filtering, MoveAndCollide, raycast, overlap. |
| level_manager | `level_manager.h`, `level_manager.c` | Level vtable + transition state machine (fade / wipe). Host-agnostic via `void* user`. |

Each module's header starts with a `DEPENDENCIES + OVERRIDES` block.
Read that block first when you copy the file.

## What's NOT in the kit (and why)

| Concern | Why it lives in the host |
|---|---|
| Game loop | A 30-line fixed-timestep accumulator. Copy from `src/game.c`, adapt. Not worth a configurable library. |
| Resource manager | At small scope, `LoadModel("path")` direct from raylib is simpler than a registry. Add a registry only if a project really needs one. |
| Stylized shading / polish passes | Stylistic effects, not infrastructure. Your project decides which to use and when to add them. |
| Camera | Game-specific (orbit, isometric, top-down, FPS). The kit doesn't impose a model. |
| Debug overlay | Inherently coupled to whatever stats you want to inspect. Write your own panels with raygui, or copy from `src/debug.c`. |
| Collision layers | Defined in your project (e.g. `include/layers.h`). The physics module operates on raw `int` bitfields. |

## Minimal project layout

```
my_game/
  lib/                  ← copy from this kit
    arena.h
    renderer.h, renderer.c
    physics.h, physics.c
    level_manager.h, level_manager.c

  externals/             ← copy these dependencies
    rmem.h              (single-header arena impl, already wrapped by arena.h)
    rmem_impl.c         (or define RMEM_IMPLEMENTATION in one .c)

  include/              ← your project headers
    config.h            ← screen size, FPS, your overrides for kit defaults
    layers.h            ← your collision layer bitflags
    game.h              ← your Game struct

  src/                  ← your project code
    main.c              ← malloc Game → init → run → shutdown → free
    game.c              ← game loop, subsystem wiring
    levels/...

  CMakeLists.txt
  assets/
```

CMake: just `file(GLOB SOURCES lib/*.c src/*.c)` and
`include_directories(lib include)`. Link raylib.

## A new project in seven steps

1. **Copy `lib/`** into your project. Copy `rmem.h` to `externals/`.
2. **Create your `Game` struct** with the kit subsystems by value:
   ```c
   typedef struct Game {
       MemArena permanent, level, scratch;
       Renderer     renderer;
       PhysWorld    physWorld;
       LevelManager levelMgr;
       /* + your camera, your render glue, your gameplay state */
   } Game;
   ```
3. **Define collision layers** in `include/layers.h`:
   ```c
   #define LAYER_PLAYER  (1 << 0)
   #define LAYER_ENEMY   (1 << 1)
   #define LAYER_SCENERY (1 << 2)
   #define MASK_ALL      0xFF
   ```
4. **Override kit defaults** in `config.h` (optional):
   ```c
   #define RENDERER_MAX_RENDERABLES 512
   #define MAX_COLLIDERS            512
   ```
   Include `config.h` **before** the kit headers in your `Game` header
   so the `#ifndef` guards see the overrides.
5. **Write a level** as a struct of function pointers:
   ```c
   static void Init(void* user) { Game* g = user; /* ... */ }
   static void Update(void* user, float dt) { Game* g = user; /* ... */ }
   /* etc. */
   Level LEVEL_MAIN = { .name = "Main", .Init = Init, .Update = Update, ... };
   ```
6. **Wire the loop** in `game.c` — pattern from this project:
   ```c
   LevelManagerInit(&game->levelMgr, game, &LEVEL_MAIN);
   game->levelMgr.onSwap = onLevelSwap;  /* host hook for ArenaReset etc. */

   while (!WindowShouldClose() && game->running) {
       float dt = GetFrameTime();
       LevelManagerUpdate(&game->levelMgr, dt);

       game->accumulator += dt;
       while (game->accumulator >= FIXED_TIMESTEP) {
           RendererPreUpdate(&game->renderer);
           LevelManagerProcessInput(&game->levelMgr);
           LevelManagerUpdateLevel(&game->levelMgr, FIXED_TIMESTEP);
           PhysicsUpdate(&game->physWorld, NULL, NULL);
           game->accumulator -= FIXED_TIMESTEP;
       }
       float alpha = game->accumulator / FIXED_TIMESTEP;
       ArenaReset(&game->scratch);

       BeginDrawing();
           ClearBackground(...);
           RendererBuildDrawList(&game->renderer, camera, alpha);
           BeginMode3D(camera);
               RendererDraw3D(&game->renderer);
               LevelManagerRender3D(&game->levelMgr, alpha);
           EndMode3D();
           LevelManagerRenderHUD(&game->levelMgr, alpha);
           LevelManagerRender(&game->levelMgr);   /* transition overlay last */
       EndDrawing();
   }
   ```
7. **Apply your shader to models** before registering:
   ```c
   Model m = LoadModel("...");
   for (int i = 0; i < m.materialCount; i++)
       m.materials[i].shader = myShader;
   RenderHandle h = RendererRegister(&game->renderer, m, materialID);
   ```

That's the whole picture. The kit is intentionally small — the bulk of
any game lives in your `src/` folder.

## Reference implementation

This repo (Prototype Horde — a 3D *Boxhead 2Play* clone) is a worked
example. Look at:

- `src/main.c` — the malloc-free pattern
- `src/game.c` — the loop and subsystem wiring
- `src/level_sandbox.c` — a level that registers models and colliders
- `include/config.h` — host-side overrides for the kit
- `include/layers.h` — collision layer definitions
