# PrototypeHorde

## Project Overview

C99 game engine built with Raylib 5.5, following the architecture from *Game Programming in C++* by Sanjay Madhav and *Practical C++ Game Programming with Data Structures and Algorithms* by Zhenyu George Li, Charles Shih-I Yeh and it Knight framework, adapted to C99. The goal is learning engine architecture — understanding *why* every decision is made, not just how.

## Architecture Summary

**No scene root.** Actors live in a flat array (`Game.actors[]`). The scene graph only exists *within* each actor (components as children of the actor's root SceneComponent). This is Madhav's approach — Knight uses a full scene tree, but we chose flat lists for simplicity and performance.

**Subsystems own their registrants:**
- `Renderer` owns a list of `MeshComponent*` (for drawing)
- `PhysWorld` owns lists of `BoxComponent*` and `SphereComponent*` (for collision)
- Components auto-register on creation, auto-unregister on destroy via callbacks

**Game is the central hub:**
```
Game
├── MemorySystem memory      (rmem pools)
├── LevelManager levelMgr    (level transitions with animation)
├── Renderer renderer        (mesh registry, frustum cull, draw pipeline)
├── PhysWorld physWorld       (collision registry, queries)
├── Actor* actors[]           (flat list, max 512)
└── Actor* pendingActors[]    (added during update, flushed after)
```

## Coordinate System

Raylib/OpenGL right-handed, Y-up:
- **Forward = -Z** (into screen)
- **Right = +X**
- **Up = +Y**

`SCENE_COMPONENT_GetForward()` returns negated column 2 of worldTransform. Positive Y rotation = counterclockwise when viewed from above. This aligns with Raylib and glTF/Blender model import.

## Component System

### Embedding Pattern (C inheritance via offset-0 struct)

Components use struct embedding at offset 0 for polymorphism. Cast `Component*` to any derived type safely if `comp->type` matches.

```
MeshComponent                CameraTPS
└─ SceneComponent base  └─ CameraComponent base
   └─ Component base            └─ SceneComponent base
                                    └─ Component base

BoxComponent                 MoveComponent
└─ Component base            └─ Component base
```

Components that need their own transform (mesh, camera) embed `SceneComponent`. Components that only need logic (move, box, sphere) embed `Component` directly.

### Component Types and Update Order

| Type | UpdateOrder | Embeds | Auto-registers with |
|------|-------------|--------|---------------------|
| COMPONENT_TYPE_MOVE | 10 | Component | — |
| COMPONENT_TYPE_MESH | 200 | SceneComponent | Renderer |
| COMPONENT_TYPE_CAMERA | 250 | SceneComponent | — |
| COMPONENT_TYPE_BOX | 300 | Component | PhysWorld |
| COMPONENT_TYPE_SPHERE | 300 | Component | PhysWorld |

Lower updateOrder = runs first. Move (10) runs before Mesh (200), so position is final before rendering reads it. Box/Sphere (300) run after movement to recompute world bounds.

### Auto-Registration Pattern

Every component that needs a centralized system follows this pattern:
```c
// In Create:
RENDERER_AddMesh(&owner->game->renderer, self);    // or PHYS_WORLD_AddBox, etc.

// In Destroy callback:
RENDERER_RemoveMesh(&mc->base.base.owner->game->renderer, mc);
```

The Destroy callback is set during Create. `COMPONENT_Destroy()` calls it before removing from actor.

## Memory System

Uses rmem.h (single-header pool allocator):
- **Actor pool**: `ObjPool` — fixed-size blocks for Actor structs (512 max)
- **Component pool**: `MemPool` — variable-size allocations for components (128 KB)
- No general heap allocation for game objects during gameplay

```c
Actor* actor = MEMORY_AllocActor(&game->memory);
Component* comp = MEMORY_AllocComponent(&game->memory, sizeof(BoxComponent));
```

## Rendering Pipeline

Each frame (`RENDERER_DrawFrame`):
1. Compute View × Projection matrix from Camera3D
2. Extract 6 frustum planes (Gribb & Hartmann method)
3. For each registered MeshComponent:
   - Skip if not visible or owner inactive
   - Transform `localBB` to world AABB via `COLLISION_TransformAABB` (8-corner method)
   - Test world AABB against frustum (positive-vertex test)
   - If visible, add to `drawList` with distance²
4. Sort `drawList` by `Material*` pointer (batch state changes)
5. `BeginMode3D` → draw sorted list → level's `Render3D` → `EndMode3D`
6. 2D: HUD → pause overlay → debug overlay → transition overlay

`localBB` in MeshComponent is for rendering only (frustum culling). BoxComponent's `objectBox` is for collision only. Same mesh can have different collision and rendering bounds.

## Collision System

### Primitives

Uses Raylib's collision functions directly:
- `CheckCollisionBoxes` — AABB vs AABB
- `CheckCollisionSpheres` — sphere vs sphere
- `CheckCollisionBoxSphere` — cross-type
- `GetRayCollisionBox` / `GetRayCollisionSphere` — raycasts

Our `collision.h` only adds `COLLISION_TransformAABB` (8-corner local→world transform) which Raylib doesn't provide.

### PhysWorld Queries

```c
// Raycast — closest hit within maxDistance
bool PHYS_WORLD_RayCast(world, ray, maxDistance, &outColl);

// Brute-force pairwise — all types cross-tested
PHYS_WORLD_TestPairwise(world, callbackFn);

// Sweep-and-prune on X axis — box-box optimized, cross-type brute force
PHYS_WORLD_TestSweepAndPrune(world, callbackFn);
```

`CollisionInfo.collider` is `Component*` — check `collider->type` to determine if it's `COMPONENT_BOX` or `COMPONENT_SPHERE`, then cast.

### BoxComponent vs MeshComponent.localBB

These serve different purposes:
- `BoxComponent.objectBox`: collision volume, set manually, optional per actor, registered with PhysWorld
- `MeshComponent.localBB`: rendering bounds for frustum culling, computed automatically from mesh data, registered with Renderer

An actor can have a mesh without collision (decorative), collision without mesh (trigger), or both with different bounds.

## Scene Graph (within each Actor)

```
Actor
└─ root (SceneComponent) ─── actor's position/rotation/scale in world
    ├─ MeshComponent.scene ── mesh offset relative to actor
    ├─ CameraComponent.scene ── camera offset relative to actor
    └─ (other scene children)
```

Transform propagation: parent dirty → all children dirty. `SCENE_COMPONENT_ComputeWorldTransform` resolves lazily (walks up to root, multiplies local transforms down). No global scene root connecting actors — actors are independent.

## Level System

Levels are static function-table structs (not heap-allocated):
```c
Level LEVEL_3 = {
    .name = "Level 3 - Components",
    .Init = Level3_Init,
    .Shutdown = Level3_Shutdown,
    .ProcessInput = Level3_ProcessInput,
    .Render3D = Level3_Render3D,
    .RenderHUD = Level3_RenderHUD,
};
```

`LevelManager` handles transitions with animated fade-out/fade-in. `GAME_ChangeLevel` triggers: fade out → destroy all actors → call new level Init → fade in.

## File Inventory (40 files)

### Core
- `main.c` — entry point
- `game.h/c` — Game struct, game loop, actor management
- `memory.h/c` — pool allocators wrapping rmem
- `rmem.h` — third-party single-header memory pool library

### Actor-Component
- `actor.h/c` — Actor struct, lifecycle, component management
- `component.h/c` — Component base, type enum, init/destroy
- `spatial_component.h/c` — Scene graph node, transform hierarchy, dirty flags

### Components
- `mesh_component.h/c` — renderable mesh, embeds SpatialComponent, auto-registers with Renderer
- `move_component.h/c` — forward/angular speed, applies to actor root
- `camera_component.h/c` — wraps Camera3D, embeds SpatialComponent
- `camera_tps.h/c` — third-person spring camera, extends CameraComponent
- `box_component.h/c` — AABB collider, auto-registers with PhysWorld
- `sphere_component.h/c` — sphere collider, auto-registers with PhysWorld

### Systems
- `renderer.h/c` — mesh registry, frustum culling, material sorting, draw pipeline
- `phys_world.h/c` — collider registry, pairwise/sweep-and-prune/raycast queries
- `collision.h/c` — AABB 8-corner transform (supplements Raylib's collision API)

### Level Management
- `level.h` — Level typedef (function table)
- `level_manager.h/c` — level transitions with animated fade
- `level_1.c` through `level_5.c` — demo levels

### Debug
- `debug.h/c` — F1 overlay with FPS graph, timing, actors, rendering stats, memory stats

## Naming Conventions

- Types: `PascalCase` — `MeshComponent`, `PhysWorld`, `Actor`
- Functions: `MODULE_FunctionName` — `ACTOR_Init`, `MESH_COMPONENT_Create`, `PHYS_WORLD_RayCast`
- Constants/macros: `UPPER_SNAKE_CASE` — `GAME_MAX_ACTORS`, `COMPONENT_MESH`
- Local variables: `camelCase`
- Struct fields: `camelCase`
- Static/internal functions: `PascalCase` without module prefix (file scope)
- Enum values: `MODULE_VALUE` — `ACTOR_STATE_ACTIVE`, `COMPONENT_MESH`

## Key Architectural Decisions (and Why)

1. **Flat actor list, no scene root** — Madhav pattern. Simpler, faster iteration. Scene graph only within actors. If inter-actor hierarchy needed later, use `SCENE_COMPONENT_AttachChild` between two actor roots.

2. **SceneComponent embedding vs Component embedding** — Only components that need their own transform in the scene graph embed SceneComponent. Box/Sphere colliders use the actor's root transform directly, so they embed Component.

3. **Madhav BoxComponent over Knight Actor-embedded BB** — Separation of concerns. Not every rendered thing collides, not every collidable thing renders. PhysWorld centralizes collision queries efficiently.

4. **Raylib collision functions over custom implementations** — `CheckCollisionBoxes`, `GetRayCollisionBox`, etc. are already implemented and tested. We only add `COLLISION_TransformAABB` which Raylib lacks.

5. **Frustum culling in Renderer, not in a RenderPass system** — Knight has an abstract RenderPass hierarchy for multi-pass rendering (depth, shadow, lit). We use Raylib's immediate mode, so a single draw pass with frustum cull + material sort is sufficient.

6. **localBB separate from BoxComponent** — MeshComponent.localBB is for rendering (frustum culling), computed automatically from mesh data. BoxComponent.objectBox is for gameplay collision, set manually, lives in PhysWorld. Different purposes, different lifecycles.

## Response Preferences

- **Direct programming question** → Direct answer + justification
- **Architecture/concept question** → Conceptual explanation, minimal code, alternatives presented
- **Task breakdown request** → Step-by-step breakdown + reasoning behind each step, so I can learn to do it myself

The goal is always to understand *what* I'm doing and *why*.

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

## Reference Sources

- **Madhav**: *Game Programming in C++* — Actor-Component pattern, PhysWorld, BoxComponent, Renderer mesh registry
- **Knight**: C++/Raylib engine — RenderPass system, render queues (Background/Geometry/AlphaBlend/Overlay), frustum culling in BuildRenderQueue, scene graph with WorldBoundingBox
- Decisions are documented comparing both approaches, with rationale for which we chose
- Reference projects and books are in reference subfolder
- \reference\GameProgrammingC++
- \reference\Knight