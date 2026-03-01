# TPS Engine — Architecture

## Overview

A custom third-person shooter engine written in C using Raylib for windowing, rendering, and input. The engine follows a component-based architecture with pool-based memory management, a fixed-timestep game loop, and modular subsystems.

The project is both a playable game prototype ("Prototype Horde") and a learning exercise in engine architecture, graphics programming, and systems-level C development.

---

## Module Map

```
main.c
  └── Game (game.h/c) ─── Application framework, owns all subsystems
        │
        ├── MemorySystem (memory.h/c) ─── Pool allocators
        │     ├── ObjPool  → Fixed-size Actor allocation (O(1))
        │     └── MemPool  → Variable-size Component allocation (bucket free lists)
        │
        ├── Renderer (renderer.h/c) ─── Frustum culling, draw-list sorting, frame output
        │     ├── FrustumPlane[6]  → Gribb-Hartmann extraction from view-projection
        │     ├── DrawEntry[]      → Sorted draw list (by material pointer)
        │     └── MeshComponent*[] → Registered renderable meshes
        │
        ├── PhysWorld (physics_world.h/c) ─── Spatial queries and collision detection
        │     ├── BoxComponent*[]    → Registered AABB colliders
        │     ├── SphereComponent*[] → Registered sphere colliders
        │     ├── RayCast / RayCastIgnore → Ray queries
        │     ├── OverlapSphere / OverlapBox → Area queries
        │     └── TestSweepAndPrune → Broad-phase pair testing
        │
        ├── LevelManager (level_manager.h/c) ─── Level lifecycle and transitions
        │     ├── Level* activeLevel   → Currently running level
        │     ├── Level* pendingLevel  → Queued for transition swap
        │     └── Transition FSM       → IDLE → FADING_OUT → swap → FADING_IN → IDLE
        │
        └── Actor*[] (actor.h/c) ─── Active entity list
              │
              └── Actor
                    ├── SceneComponent root ─── Transform hierarchy root
                    │     ├── position, rotation, scale (local)
                    │     ├── localTransform, worldTransform (cached, dirty-flag)
                    │     └── children[] → child SceneComponents
                    │
                    ├── Component*[] ─── Sorted by updateOrder
                    │     ├── MeshComponent (scene_component.h → mesh_component.h/c)
                    │     │     └── Mesh*, Material*, tint, localBB, visible
                    │     ├── MoveComponent (move_component.h/c)
                    │     │     └── forwardSpeed, angularSpeed, strafeSpeed
                    │     ├── BoxComponent (box_component.h/c)
                    │     │     └── objectBox (local AABB), worldBox (transformed)
                    │     ├── SphereComponent (sphere_component.h/c)
                    │     │     └── offset, radius, worldCenter, worldRadius
                    │     ├── CameraComponent (camera_component.h/c)
                    │     │     └── Camera3D cam, SceneComponent scene
                    │     └── CameraTPS (camera_tps.h/c) extends CameraComponent
                    │           └── Spring physics, wall collision, ideal position tracking
                    │
                    └── Callbacks: Update, Input, Destroy (virtual function pointers)
```

---

## Source Files

### Implemented (29 files)

| Module | Header | Source | Purpose |
|--------|--------|--------|---------|
| Component | `component.h` | `component.c` | Base type for all components |
| SceneComponent | `scene_component.h` | `scene_component.c` | Transform hierarchy with dirty-flag caching |
| Actor | `actor.h` | `actor.c` | Entity that owns components and a root transform |
| MeshComponent | `mesh_component.h` | `mesh_component.c` | Renderable mesh with material and tint |
| MoveComponent | `move_component.h` | `move_component.c` | Basic linear/angular/strafe movement |
| BoxComponent | `box_component.h` | `box_component.c` | AABB collider with world-space transform |
| SphereComponent | `sphere_component.h` | `sphere_component.c` | Sphere collider with scaled world radius |
| CameraComponent | `camera_component.h` | `camera_component.c` | Base camera with Raylib Camera3D |
| CameraTPS | `camera_tps.h` | `camera_tps.c` | Third-person spring camera with wall avoidance |
| Collision | `collision.h` | `collision.c` | Stateless geometric tests (AABB, sphere, segment) |
| PhysWorld | `physics_world.h` | `physics_world.c` | Spatial queries, broad-phase, raycasting |
| Memory | `memory.h` | `memory.c` | Pool allocators (ObjPool + MemPool via rmem) |
| Renderer | `renderer.h` | `renderer.c` | Frustum culling, material sort, frame rendering |
| Game | `game.h` | `game.c` | Main loop, actor lifecycle, subsystem orchestration |
| LevelManager | `level_manager.h` | `level_manager.c` | Level loading, transitions, swap logic |
| Level | `level.h` | — | Level interface (function pointer table) |
| Entry | — | `main.c` | Application entry point |
| Debug | `debug.h` | `debug.c` | Debug overlay (to be modularized in Phase 0) |
| — | `rmem.h` | — | Third-party memory pool library (raylib ecosystem) |

---

## C-Style Inheritance Model

The engine uses struct embedding for inheritance. The base type is always the **first field**, allowing safe casting between base and derived pointers.

```
Component                     ← Base type (component.h)
  │
  ├── SceneComponent          ← Adds transform hierarchy
  │     .base = Component
  │     │
  │     ├── MeshComponent     ← Renderable
  │     │     .scene = SceneComponent
  │     │
  │     └── CameraComponent   ← Camera view
  │           .scene = SceneComponent
  │           │
  │           └── CameraTPS   ← Third-person follow camera
  │                 .base = CameraComponent
  │
  ├── MoveComponent           ← Movement speeds
  │     .base = Component
  │
  ├── BoxComponent            ← AABB collider
  │     .base = Component
  │
  └── SphereComponent         ← Sphere collider
        .base = Component
```

**Field naming convention:**
- `.base` → inheriting from Component
- `.scene` → inheriting from SceneComponent
- `.base` → inheriting from CameraComponent (in CameraTPS)

**Safe casting example:**
```c
MeshComponent* mc = MESH_COMPONENT_Create(owner, mesh, material);
Component* comp = &mc->scene.base;  // Upcast: always safe (first-field chain)
MeshComponent* back = (MeshComponent*)comp;  // Downcast: valid if comp->type == COMPONENT_TYPE_MESH
```

---

## Game Loop

Implements Glenn Fiedler's "Fix Your Timestep!" pattern.

```
GAME_Run()
│
├── while (!quit)
│     │
│     ├── frameTime = GetFrameTime()       // Variable: real wall-clock delta
│     ├── clamp(frameTime, MAX_DELTA_TIME)  // Spiral-of-death protection
│     │
│     ├── LEVEL_MGR_Update()               // Transition state machine
│     ├── DEBUG_Update()                   // Debug overlay logic
│     ├── GAME_ProcessInput()              // Per-frame input (responsive)
│     │     ├── Global keys (P=pause, ESC=quit)
│     │     ├── Level::ProcessInput()
│     │     └── Actor::ProcessInput() for all actors
│     │
│     ├── accumulator += frameTime
│     │
│     ├── SavePrevState() for all actors   // Snapshot for interpolation
│     │
│     ├── while (accumulator >= FIXED_TIMESTEP)     // Drain in fixed chunks
│     │     ├── GAME_FixedUpdate(FIXED_TIMESTEP)
│     │     │     ├── Phase 1: ACTOR_Update() for all active actors
│     │     │     ├── Phase 2: Flush pending actors to active list
│     │     │     └── Phase 3: Destroy dead actors (reverse iteration)
│     │     └── accumulator -= FIXED_TIMESTEP
│     │
│     ├── alpha = accumulator / FIXED_TIMESTEP       // Interpolation factor
│     ├── InterpolateForRender(alpha) for all actors // Smooth visual state
│     │
│     ├── RENDERER_DrawFrame()                       // Cull + sort + draw
│     │
│     └── RestoreFromInterpolation() for all actors  // Back to true physics state
```

**Timing constants:**
- `UPDATE_RATE` = 60 Hz (physics/logic)
- `FIXED_TIMESTEP` = 1/60 ≈ 16.67ms
- `RENDER_FPS` = 30 (target framerate, independent from update rate)
- `MAX_DELTA_TIME` = 0.25s (caps at ~15 ticks max per frame)

---

## Actor Lifecycle

```
Actor creation:
  MEMORY_AllocActor() → ACTOR_Create() → GAME_AddActor()
    │
    ├── If mid-update (updatingActors == true):
    │     → pendingActors[] (deferred insertion)
    │
    └── If not updating:
          → actors[] (immediate insertion)

Pending → Active promotion:
  GAME_FixedUpdate Phase 2:
    for each pending actor:
      ACTOR_ComputeWorldTransform()
      move to actors[]

Actor destruction:
  Set actor->state = ACTOR_STATE_DEAD
  GAME_FixedUpdate Phase 3 (reverse iteration):
    ACTOR_Destroy() → calls Destroy callbacks → GAME_RemoveActor()
      → O(1) swap-remove from actors[]
      → COMPONENT_Destroy() for each component
      → MEMORY_FreeActor()
```

---

## Memory Architecture

```
MemorySystem
├── ObjPool (actorPool)
│     Purpose: Fixed-size allocation for Actor structs
│     Backing: rmem ObjPool (free-list based)
│     Perf:    O(1) alloc, O(1) free, zero fragmentation
│     Size:    GAME_MAX_ACTORS * sizeof(Actor)
│
└── MemPool (componentPool)
      Purpose: Variable-size allocation for all Component types
      Backing: rmem MemPool (bucket-based free lists)
      Perf:    O(1) alloc from matching bucket, may fragment over time
      Size:    Configurable (currently sized for ~512 components)
```

**Allocation flow:**
```c
// Actor: fixed-size pool
Actor* a = MEMORY_AllocActor(&game->memory);

// Component: variable-size pool (caller specifies size)
MeshComponent* mc = MEMORY_AllocComponent(&game->memory, sizeof(MeshComponent));

// Deallocation
MEMORY_FreeActor(&game->memory, a);
MEMORY_FreeComponent(&game->memory, mc);
```

**Rule:** No raw `malloc` for actors or components during gameplay. Pool allocators prevent fragmentation and provide predictable allocation times.

---

## Transform Hierarchy (SceneComponent)

Each Actor has an embedded `SceneComponent root` that serves as the transform hierarchy root. Components that need a position in 3D space (MeshComponent, CameraComponent) inherit from SceneComponent and attach as children.

```
Actor.root (SceneComponent)
  ├── MeshComponent.scene (child)
  ├── CameraComponent.scene (child, via CameraTPS)
  └── (future: other positioned components)
```

**Dirty flag optimization:**
- Setting position/rotation/scale marks the node `isDirty = true`
- `ComputeWorldTransform()` only recomputes if dirty
- Marking dirty propagates to all children (cascade)
- World transform = parent.worldTransform × local transform

**Interpolation for rendering:**
- `SavePrevState()` — stores current position/rotation before physics tick
- `InterpolateForRender(alpha)` — lerps between prev and current for smooth rendering
- `RestoreFromInterpolation()` — reverts to true physics state after rendering

---

## Rendering Pipeline

```
RENDERER_DrawFrame()
│
├── Begin 3D mode with active camera
│
├── Build draw list:
│     for each registered MeshComponent:
│       ├── Skip if !visible
│       ├── Compute world AABB (COLLISION_TransformAABB)
│       ├── Frustum cull (RENDERER_IsAABBInFrustum)
│       └── Add to drawList[] if visible
│
├── Sort draw list by material pointer
│     (reduces GPU state changes for same-material meshes)
│
├── Draw sorted meshes:
│     for each DrawEntry:
│       DrawMesh(mesh, material, worldTransform)
│
├── Level::Render3D()      // Level-specific 3D drawing (grid, gizmos)
├── DEBUG_Render3D()       // Debug 3D overlays (collider wireframes)
│
├── End 3D mode
│
├── Begin 2D overlay
├── Level::RenderHUD()     // Level-specific HUD
├── DEBUG_Render()         // Debug 2D panels
├── LEVEL_MGR_Render()     // Transition effects (fade, wipe)
└── End drawing
```

**Frustum culling:** Uses Gribb-Hartmann plane extraction from the view-projection matrix. Each AABB is tested against 6 frustum planes using the positive-vertex test (Assarsson & Möller).

---

## Physics / Collision Architecture

### Registered Colliders

PhysWorld maintains flat arrays of collider pointers. Components register/unregister themselves during Create/Destroy.

```
PhysWorld
├── boxes[512]    → BoxComponent* (AABB colliders)
├── spheres[512]  → SphereComponent* (sphere colliders)
└── (future: capsules[256])
```

### Query Types

| Query | Function | Use Case |
|-------|----------|----------|
| Raycast | `PHYS_WORLD_RayCast` | Hitscan weapons, line-of-sight |
| Raycast (ignore actor) | `PHYS_WORLD_RayCastIgnore` | Shoot from player, skip self |
| Overlap sphere | `PHYS_WORLD_OverlapSphere` | Area damage, proximity detection |
| Overlap box | `PHYS_WORLD_OverlapBox` | Zone triggers, area queries |
| Sphere cast | `PHYS_WORLD_SphereCast` | CCD for projectiles |
| Pairwise test | `PHYS_WORLD_TestPairwise` | Brute-force N² pair check |
| Sweep-and-prune | `PHYS_WORLD_TestSweepAndPrune` | O(n log n) broad-phase |

### Collision Utilities (collision.h)

Stateless geometric functions, no system state:

- `COLLISION_TransformAABB` — Transform local AABB by world matrix (8-corner test)
- `COLLISION_BoxVsBox` — AABB penetration depth + normal
- `COLLISION_BoxVsSphere` — Box-sphere penetration
- `COLLISION_SphereVsSphere` — Sphere-sphere penetration
- `COLLISION_ClosestPointOnSegment` — Foundation for capsule tests
- `COLLISION_PointToAABBDistSq` — Squared distance for comparisons

---

## Level System

Levels are static structs with function pointer tables (no dynamic allocation):

```c
typedef struct Level {
    const char* name;
    LevelInitFn Init;           // Spawn actors, set up scene
    LevelShutdownFn Shutdown;   // Cleanup level-specific resources
    LevelInputFn ProcessInput;  // Level-specific input handling
    LevelRender3DFn Render3D;   // Level-specific 3D drawing
    LevelRenderHUDFn RenderHUD; // Level-specific HUD overlay
} Level;
```

**Transition state machine (LevelManager):**
```
IDLE → TransitionTo() → FADING_OUT → screen covered → ApplySwap() → FADING_IN → IDLE
                                                         │
                                                         ├── RemoveAllActors()
                                                         ├── activeLevel->Shutdown()
                                                         ├── Reset game state
                                                         ├── Set new activeLevel
                                                         └── newLevel->Init()
```

Built-in transition effects: `TRANSITION_Fade`, `TRANSITION_WipeLeft`, `TRANSITION_WipeRight`.

---

## Naming Conventions

```
Enums:         MODULE_ENUM_VALUE         (ACTOR_STATE_ACTIVE, COMPONENT_TYPE_MESH)
Type enums:    MODULE_TYPE_NAME          (ACTOR_TYPE_TPS, COMPONENT_TYPE_CAMERA_TPS)
Structs:       PascalCase                (PhysWorld, CameraTPS, MeshComponent)
Functions:     MODULE_VerbNoun           (ACTOR_GetForward, RENDERER_DrawFrame)
Constants:     MODULE_CONSTANT           (GAME_MAX_ACTORS, PHYS_WORLD_MAX_BOXES)
Callbacks:     ModuleVerbFn              (ActorUpdateFn, CollisionPairFn, TransitionEffectFn)
Static/local:  No prefix for locals, s_ or S prefix for file-scope statics
Lifecycle:     MODULE_Init/Shutdown      (embedded subsystems owned by Game)
               MODULE_Create/Destroy     (heap-allocated from pools)
Inheritance:   .base for Component       (.scene for SceneComponent)
Guards:        #pragma once
```

---

## Component Update Order

Components within an Actor are sorted by `updateOrder` (insertion sort on add). Lower values tick first:

| Order | Component | Rationale |
|-------|-----------|-----------|
| 10 | MoveComponent | Movement before everything else |
| 200 | MeshComponent | Visuals after logic |
| 250 | CameraComponent / CameraTPS | Camera follows post-movement position |
| 300 | BoxComponent / SphereComponent | Colliders after transforms settle |

---

## Key Design Patterns

| Pattern | Where | Why |
|---------|-------|-----|
| Struct embedding (C inheritance) | All components | Polymorphism without vtables |
| Virtual function pointers | Component, Actor | Custom Update/Input/Destroy per type |
| Dirty flag | SceneComponent | Avoid redundant matrix recomputation |
| Fixed timestep + interpolation | GAME_Run | Deterministic physics, smooth rendering |
| Double-buffer actors | Game (actors + pending) | Safe iteration during updates |
| Swap-remove | Actor/collider arrays | O(1) removal from unordered lists |
| Pool allocation | Memory system | O(1) alloc, zero fragmentation |
| Sweep-and-prune | PhysWorld broad-phase | O(n log n) vs O(n²) for pair testing |
| Material sort | Renderer draw list | Minimize GPU state changes |
| Frustum culling | Renderer | Skip off-screen meshes entirely |
| Spring physics | CameraTPS | Smooth camera following without oscillation |

---

## Dependencies

```
main.c → game.h → level_manager.h → level.h
                 → renderer.h      → mesh_component.h → scene_component.h → component.h
                 → physics_world.h → box_component.h  → component.h
                 │                 → sphere_component.h → component.h
                 │                 → collision.h
                 → memory.h        → rmem.h
                 → actor.h         → scene_component.h

camera_tps.h → camera_component.h → scene_component.h
move_component.h → component.h
```

**External dependency:** Raylib 5.5 (windowing, rendering, input, audio primitives).