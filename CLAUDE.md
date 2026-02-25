# PrototypeHorde

C99 ECS-based 3D game engine using Raylib 5.5. Targets Windows desktop (OpenGL 3.3 + GLFW) and RG35XX ARM handheld (OpenGL ES 3.0 + SDL2).

## Build Commands

```batch
# Windows (recommended)
build-windows.bat
# Output: build-windows/Release/Prototype-Horde.exe

# RG35XX cross-compile (requires Arm GNU Toolchain aarch64-none-linux-gnu v14.3+)
build-rg35xx.bat
# Output: build-rg35xx/rg35xx/Prototype-Horde

# Manual CMake (Windows)
mkdir build-windows
cd build-windows
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Raylib 5.5 is fetched automatically by CMake via FetchContent — no manual install needed.

## Architecture

### Game Loop (`src/game.c`)
- Fixed 60Hz physics/update timestep (`FIXED_TIMESTEP = 1/60`)
- 30 FPS render target
- Max delta time clamped to 0.25s
- Order: input → actor updates → physics → render

### Entity-Component System
- **Actors** (`src/actor.c`): Pre-allocated pool of 512. Create with `ACTOR_Create()`, destroy with `ACTOR_Destroy()`.
- **Components** (`src/component.c`): Attached to actors, updated by `updateOrder` priority. 128KB arena allocator.
- Component types: `COMPONENT_TYPE_SCENE`, `MESH`, `MOVE`, `CAMERA`, `CAMERA_TPS`, `BOX`, `SPHERE`

### Key Subsystems
| File | Responsibility |
|------|---------------|
| `src/game.c` / `include/game.h` | Game loop, global timing constants |
| `src/actor.c` / `include/actor.h` | Actor pool, lifecycle |
| `src/component.c` / `include/component.h` | Component base, registration |
| `src/scene_component.c` | Hierarchical transforms, dirty-flag propagation |
| `src/renderer.c` | Frustum culling, distance-sorted draw list |
| `src/physics_world.c` | Box/sphere colliders, raycasts, overlap queries |
| `src/level_manager.c` | Level transitions (fade, wipe effects) |
| `src/memory.c` / `include/memory.h` | Actor pool + 128KB component arena |
| `src/debug.c` | On-screen HUD: memory, physics, render stats |
| `CMakeLists.txt` | Build config, Raylib fetch, platform flags |

## Coding Conventions

**Functions**: `MODULE_FunctionName()` — module prefix in UPPER_SNAKE, name in PascalCase.
```c
ACTOR_Create(), PHYS_WORLD_Update(), LEVEL_MGR_RequestTransition()
```

**Types**:
- Structs: `PascalCase` (`Actor`, `SceneComponent`, `PhysicsWorld`)
- Enums/constants: `UPPER_SNAKE_CASE` (`ACTOR_STATE_ACTIVE`, `COMPONENT_TYPE_MESH`)
- Function pointers: `PascalCaseFn` (`ActorUpdateFn`)

**Variables**: `camelCase` for locals and struct fields.

**Headers**: `#pragma once`. Use forward declarations for opaque pointers; minimize includes.

**Logging**: Use `TraceLog(LOG_INFO/WARNING/ERROR, ...)` — never `printf`.

**Assertions**: `assert()` for preconditions and invariants.

**Memory**: No `malloc`/`free` at runtime. Use the actor pool and component arena. Static max sizes are defined as constants (e.g., `GAME_MAX_ACTORS 512`).

**Error handling**: Return `bool` from init functions, log via `TraceLog`, clean up on failure.

## Testing

No automated test suite. Validate using:
- Sandbox level (`src/level_sandbox.c`) for ad-hoc testing
- Debug HUD (`src/debug.c`) for memory, physics, and render stats at runtime
