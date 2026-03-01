# TPS Engine — Development Plan

---

## Current Status

### What's Built (Foundation)

The engine has a working component-based architecture with the following systems operational:

- **Component system** — C-style inheritance via struct embedding, virtual function pointers, update-order sorting
- **Scene graph** — Transform hierarchy with dirty-flag caching, parent-child propagation, render interpolation
- **Actor management** — Pool-allocated entities with double-buffered update (pending/active), swap-remove O(1)
- **Rendering** — Frustum culling (Gribb-Hartmann), material-sorted draw lists, decoupled from physics rate
- **Physics** — AABB and sphere colliders, raycasting, sweep-and-prune broad-phase, overlap queries
- **Collision math** — AABB transform, box-box/box-sphere/sphere-sphere penetration, segment distance
- **Memory** — Pool allocators (ObjPool for actors, MemPool for components), zero malloc during gameplay
- **Camera** — Third-person spring camera with critically-damped following and wall collision avoidance
- **Levels** — Level interface with function pointers, LevelManager with fade/wipe transitions
- **Game loop** — Fixed-timestep with interpolation (Glenn Fiedler's "Fix Your Timestep!")
- **Debug** — Monolithic overlay (FPS, actor stats, memory usage — to be modularized)

### What Remains in Phase 0

```
 0.6  Sandbox level:
      - Development hub with spatial zones for testing each subsystem
      - Hotkey spawning (cubes, spheres, walls, ramps, wanderers, stress test)
      - Toggleable grid and collider wireframes
      - Cycled camera distances (close/medium/far)
      - Spawn tracking (clear spawned vs world geometry)
      - Extend with each new Phase

 0.7  Modularize debug system:
      - Debug Tool Registry (named, toggleable tools with hotkeys)
      - Extract current monolithic overlay into: Performance, World, Rendering, Memory tools
      - Hotkey scheme: F1 master, F2 Perf, F3 World, F4 Render, F5 Physics, F6 Memory
      - 3D visualization callbacks (wireframe colliders, contact points)
      - Toolbar HUD showing all tools with toggle state
```

**New files needed:** `debug_tools.h` (DebugTool struct + registry API)
**Estimated remaining effort:** 1-2 weeks

---

## Phase 1 — Input System

**Goal:** Decouple input from hardcoded keys, support gamepad, enable rebinding.

```
 1.1  Define InputAction struct (name, key bindings, gamepad bindings, state)
 1.2  Create InputMap (array of InputActions, load from config)
 1.3  Implement polling: IsActionPressed / IsActionDown / GetActionAxis
 1.4  Add gamepad support (axes with dead zones, button mapping)
 1.5  Refactor PlayerInput callbacks to use InputActions
 1.6  Add input buffering (queue recent presses for combo-friendly timing)
```

**New files:** `input_system.h`, `input_system.c`
**Estimated effort:** 1 week

---

## Phase 2 — Event / Message System

**Goal:** Decouple subsystems. Publish-subscribe for game events.

```
 2.1  Define EventType enum and Event struct (type + union payload)
 2.2  Implement subscriber registry (type → callback list)
 2.3  Implement event queue (deferred dispatch per frame)
 2.4  Add immediate dispatch option for critical events
 2.5  Wire up: collision → EVENT_COLLISION, actor death → EVENT_ACTOR_DIED
 2.6  Add debug logging for events (toggle in debug overlay)
```

**New files:** `event_system.h`, `event_system.c`
**Estimated effort:** 1 week

---

## Phase 3 — Finite State Machine

**Goal:** Reusable FSM for character controllers, AI, and game state.

```
 3.1  Define State struct (name, Enter, Update, Exit function pointers)
 3.2  Define FSM struct (current, previous, timeInState)
 3.3  Implement FSM_Init, FSM_ChangeState, FSM_Update
 3.4  Add transition conditions (FSM_ChangeStateIf with predicate)
 3.5  Add state history (for "return to previous state")
 3.6  Create FSMComponent wrapper to embed in actors
```

**New files:** `fsm.h`, `fsm.c`
**Estimated effort:** 3-5 days

---

## Phase 4 — Character Controller & Player Controller

**Goal:** Player character that feels good to move in 3rd person.

```
 4.1  CharacterMovement component:
      - CapsuleComponent as primary collider (height ~1.8, radius ~0.3)
      - Gravity, ground detection (capsule cast down or raycast from base)
      - Jump with coyote time and jump buffering
      - Slope handling (slide on steep, walk on shallow)
      - Step-up for small obstacles
      - Velocity-based movement with acceleration/deceleration
      - Collision response via capsule vs environment (slide along walls)

 4.2  PlayerController component:
      - Camera-relative movement (input forward = camera forward projected on XZ)
      - Turn-to-face movement direction (smooth rotation)
      - Aim mode (strafe, character faces camera forward)
      - Integrates with InputSystem actions

 4.3  Player FSM states:
      - Idle → Run → Sprint → Aim/AimWalk → Roll/Dodge → Hurt → Dead

 4.4  Collision response:
      - Capsule slides along walls (project velocity onto surface tangent)
      - Push out of overlaps (penetration resolution)
      - Capsule vs capsule for character-to-character blocking
```

**New files:** `character_movement.h/.c`, `player_controller.h/.c`
**Dependencies:** Phase 1 (Input), Phase 3 (FSM), Phase 5 (Capsule collider)
**Estimated effort:** 2-3 weeks

---

## Phase 5 — Collision & Physics Improvements

**Goal:** Capsule collider, collision response, layers, spatial acceleration, CCD.

```
 5.1  CapsuleComponent — new collision primitive:
      - Defined by height, radius, offset, axis
      - World-space update: transform endpoints by owner matrix, scale radius
      - World AABB for broadphase
      - All tests reduce to point-segment + sphere distance
        (capsule = Minkowski sum of segment + sphere)
      - Tests: Capsule vs Plane/AABB/Sphere/Capsule, Ray vs Capsule

 5.2  Collision layers / masks:
      - 32-bit layer mask per collider
      - Layer matrix (which layers collide with which)
      - Filter in raycast and pairwise tests

 5.3  Collision response:
      - Penetration depth + direction for all primitive pairs
      - Separate "solid" (blocking) from "trigger" (overlap only) colliders
      - Callbacks: OnCollisionEnter, OnCollisionStay, OnCollisionExit

 5.4  Spatial acceleration — Uniform Grid:
      - Grid cell size = largest expected collider diameter
      - Only test colliders in same + neighboring cells
      - Replace brute-force pairwise

 5.5  Ground/wall queries:
      - ShapeCast down for ground detection
      - ShapeCast forward for wall detection
      - Surface normal + slope angle

 5.6  Continuous Collision Detection (CCD):
      - Capsule sweep (character dashes/rolls)
      - Sphere sweep (projectiles)
      - Moving AABB sweep (Minkowski sum approach)
      - TOI sorting for correct resolution order
```

**New files:** `capsule_component.h/.c`, `collision_layers.h`, `spatial_grid.h/.c`, `sweep.h/.c`
**Estimated effort:** 4-5 weeks

---

## Phase 5B — Prefab / Blueprint System

**Goal:** Data-driven actor templates. Spawn complex actors by name, not code.

```
 5B.1  Core: Prefab struct (name, type, scale, component descriptors, configure callback)
 5B.2  Instantiation: PREFAB_Spawn creates Actor, resolves components, runs Configure
 5B.3  Registry: static array, register/find by name
 5B.4  Inheritance: prefab chains (parent components created first)
 5B.5  Convenience macros: COMP_DESC_MESH, COMP_DESC_BOX_AUTO, etc.
 5B.6  Future: name-based asset resolution (Phase 6), JSON loading (Phase 9)
 5B.7  Tag bitmasks carried to Actor for runtime queries
 5B.8  Sandbox integration: prefab browser in debug overlay
```

**New files:** `prefab.h`, `prefab.c`, `prefab_examples.c`
**Dependencies:** None (works immediately)
**Enhanced by:** Phase 6 (name resolution), Phase 9 (JSON)
**Estimated effort:** 1 week

---

## Phase 6 — Asset Manager & Resource System

**Goal:** Centralized asset loading with reference counting.

```
 6.1  Asset handle system (index into typed arrays)
 6.2  Registry: hash map name → handle, reference counting, auto-unload
 6.3  Material system: material instances (share shader, differ in params)
 6.4  Level asset manifest: batch preload/release per level
 6.5  (Future) Async loading during transitions
```

**New files:** `asset_manager.h/.c`, `material_system.h/.c`
**Estimated effort:** 2 weeks

---

## Phase 7 — Skeletal Animation

**Goal:** Animated characters using Raylib's ModelAnimation support.

```
 7.1  AnimatedMeshComponent (wraps Raylib Model + ModelAnimation[])
 7.2  AnimationPlayer (play, pause, speed, loop modes)
 7.3  Animation blending (crossfade, blend trees by speed parameter)
 7.4  Animation state machine (integrates with FSM, state → clip mapping)
 7.5  Animation events (callbacks at specific frames: footsteps, muzzle flash)
 7.6  (Future) IK: foot placement, aim IK
```

**New files:** `animated_mesh.h/.c`, `animation_player.h/.c`, `animation_fsm.h/.c`
**Dependencies:** Phase 3 (FSM), Phase 6 (Assets)
**Estimated effort:** 3-4 weeks

---

## Phase 8 — Audio System

**Goal:** 3D spatial audio, sound management, music.

```
 8.1  AudioSystem core (sound pool, priority system)
 8.2  3D spatial audio (AudioSourceComponent, distance attenuation, panning)
 8.3  Sound categories (SFX, Music, UI, Ambient — independent volumes)
 8.4  Music manager (crossfade, layered music, stingers)
 8.5  Sound triggers (event-driven, animation-driven, randomized variation)
```

**New files:** `audio_system.h/.c`, `audio_source.h/.c`
**Dependencies:** Phase 2 (Events), Phase 7 (Animation events)
**Estimated effort:** 2 weeks

---

## Phase 9 — Serialization

**Goal:** Save/load scenes and game state via JSON.

```
 9.1  JSON reader/writer (cJSON integration)
 9.2  Component serialization (type → Save/Load functions)
 9.3  Actor serialization (type, transform, components)
 9.4  Level/Scene serialization (JSON → spawn actors, replace hardcoded Init)
 9.5  Prefab serialization (JSON-defined prefabs, hot-reload)
 9.6  (Future) Save game state
```

**New files:** `serialization.h/.c`, `component_factory.h/.c`
**Dependencies:** Phase 5B (Prefabs), Phase 6 (Asset names)
**Estimated effort:** 2-3 weeks

---

## Phase 10 — Rendering Improvements

**Goal:** Visual quality sufficient for a TPS.

```
 10.1  Basic lighting (directional + ambient, custom shader)
 10.2  Shadow mapping (single directional, PCF filtering)
 10.3  Skybox (cubemap, per-level)
 10.4  Particle system (emitter component, billboard quads)
 10.5  Decals (projected textures, pooled recycling)
 10.6  Post-processing (framebuffer passes: vignette, desaturation, bloom)
```

**Estimated effort:** 3-4 weeks (incremental)

---

## Phase 11 — AI Foundation

**Goal:** Basic enemy behavior for a TPS.

```
 11.1  AI perception (sight cone + raycast LOS, hearing radius, awareness levels)
 11.2  AI FSM states (Idle/Patrol → Investigate → Chase → Attack → Flee → Death)
 11.3  Navigation (steering behaviors, future: navmesh)
 11.4  AIController component (mirrors PlayerController, driven by AI decisions)
```

**New files:** `ai_perception.h/.c`, `ai_controller.h/.c`
**Dependencies:** Phase 3 (FSM), Phase 4 (CharacterMovement), Phase 5 (Raycasts)
**Estimated effort:** 3-4 weeks

---

## Phase 12 — UI System

**Goal:** Engine-level UI framework for menus, HUD, and overlays.

```
 12.1  UI core (immediate-mode layer, anchor-based layout, resolution-independent)
 12.2  Widget library (Label, Image, ProgressBar, Button, Panel, Slider, Toggle)
 12.3  HUD elements (crosshair, health bar, ammo, hit markers, damage direction, objective markers)
 12.4  Menu system (stack-based: main → pause → settings, cursor management)
 12.5  UI animation (tween system, easing functions, screen flash/shake)
 12.6  Data binding (UI observes game state via Event System)
 12.7  Debug UI (in-game console, command execution, dockable panels)
```

**New files:** `ui_system.h/.c`, `ui_widgets.h/.c`, `ui_hud.h/.c`, `ui_menu.h/.c`, `ui_tween.h/.c`
**Dependencies:** Phase 1 (Input), Phase 2 (Events)
**Estimated effort:** 3-4 weeks

---

## Dependency Graph

```
Phase 0 ─── Stabilize + Sandbox + Modular Debug
  │
  ├── Phase 1 ─── Input System
  │     │
  ├── Phase 2 ─── Event System
  │     │
  ├── Phase 3 ─── FSM
  │     │
  │     ├── Phase 4 ─── Character Controller ← (1, 3, 5)
  │     │     │
  │     ├── Phase 7 ─── Skeletal Animation ← (3, 6)
  │     │     │
  │     └── Phase 11 ── AI ← (3, 4, 5)
  │
  ├── Phase 5 ─── Collision + Capsule + CCD
  │
  ├── Phase 5B ── Prefab System (no hard deps)
  │     │
  │     ├── Enhanced by Phase 6 (name-based asset resolution)
  │     └── Enhanced by Phase 9 (JSON-defined prefabs)
  │
  ├── Phase 6 ─── Asset Manager
  │     │
  │     └── Phase 9 ─── Serialization ← (6)
  │
  ├── Phase 8 ─── Audio ← (2, 7)
  │
  ├── Phase 10 ── Rendering (parallel, incremental)
  │
  └── Phase 12 ── UI System ← (1, 2)
```

---

## Sprint Schedule

| Sprint | Phases | Duration | Milestone |
|--------|--------|----------|-----------|
| 1 | 0 (remaining) | 1-2 weeks | Sandbox level, modular debug |
| 2 | 1 + 2 | 2 weeks | Input actions + events working |
| 3 | 3 + 5.1-5.3 | 3 weeks | FSM + capsule collider + collision response |
| 4 | 4 + 5B | 3-4 weeks | **Moveable character with capsule + prefab spawning** |
| 5 | 5.4-5.6 (CCD) | 2-3 weeks | Spatial grid, sweep tests, CCD |
| 6 | 6 | 2 weeks | Asset manager, prefabs resolve by name |
| 7 | 7 | 3-4 weeks | **Animated character on screen** |
| 8 | 8 | 2 weeks | Sound effects, spatial audio |
| 9 | 10.1-10.4 | 2-3 weeks | Lighting, shadows, particles |
| 10 | 11 | 3-4 weeks | **Enemies that fight back** |
| 11 | 12.1-12.3 | 2-3 weeks | **UI framework + HUD** |
| 12 | 12.4-12.7 | 1-2 weeks | Menus, tweens, debug UI |
| 13 | 9 | 2 weeks | Serialized levels + JSON prefabs |
| 14 | 10.5-10.6 | 2 weeks | Polish: decals, post-fx |

**Total estimate: ~30-42 weeks**

### Key Milestones

- **After Sprint 4:** Character with capsule collision + prefab-based spawning
- **After Sprint 7:** Animated character in a level
- **After Sprint 10:** Vertical slice — player vs enemies in a level
- **After Sprint 12:** Proper HUD, menus, and settings screens
- **After Sprint 13:** JSON-defined levels with prefab placement

---

## File Inventory

### Implemented (29 source files + 1 third-party)

```
Core ECS:        component.h/c, actor.h/c, scene_component.h/c
Components:      mesh_component.h/c, move_component.h/c,
                 box_component.h/c, sphere_component.h/c,
                 camera_component.h/c, camera_tps.h/c
Systems:         collision.h/c, physics_world.h/c, memory.h/c, renderer.h/c
Framework:       game.h/c, level.h, level_manager.h/c, main.c
Debug:           debug.h/c
Third-party:     rmem.h (raylib memory pool library)
```

### Planned (new files per phase)

```
Phase 0:   debug_tools.h
Phase 1:   input_system.h/c
Phase 2:   event_system.h/c
Phase 3:   fsm.h/c
Phase 4:   character_movement.h/c, player_controller.h/c
Phase 5:   capsule_component.h/c, collision_layers.h, spatial_grid.h/c, sweep.h/c
Phase 5B:  prefab.h/c, prefab_examples.c
Phase 6:   asset_manager.h/c, material_system.h/c
Phase 7:   animated_mesh.h/c, animation_player.h/c, animation_fsm.h/c
Phase 8:   audio_system.h/c, audio_source.h/c
Phase 9:   serialization.h/c, component_factory.h/c
Phase 12:  ui_system.h/c, ui_widgets.h/c, ui_hud.h/c, ui_menu.h/c, ui_tween.h/c
```