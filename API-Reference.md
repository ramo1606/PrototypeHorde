# TPS Engine — API Reference

---

## Conventions

```
Naming:        MODULE_FunctionName(Module* self, ...)
Lifecycle:     MODULE_Init / MODULE_Shutdown     (subsystems owned by Game)
               MODULE_Create / MODULE_Destroy    (heap-allocated from pools)
Inheritance:   C embedding — first field IS the base type
               .base for Component, .scene for SceneComponent
Virtual:       Function pointers in struct (Update, Input, Destroy)
Forward decl:  typedef struct X X; in headers
Guards:        #pragma once
Alloc:         Pool-based via MemorySystem, never raw malloc for actors/components

Enums:         MODULE_ENUM_VALUE       (ACTOR_STATE_ACTIVE)
Type enums:    MODULE_TYPE_NAME        (COMPONENT_TYPE_MESH)
Structs:       PascalCase              (PhysWorld, CameraTPS)
Functions:     MODULE_VerbNoun         (ACTOR_GetForward, RENDERER_DrawFrame)
Constants:     MODULE_CONSTANT         (GAME_MAX_ACTORS)
Callbacks:     ModuleVerbFn            (ActorUpdateFn, CollisionPairFn)
```

---

## IMPLEMENTED MODULES

These modules exist in the codebase and are fully functional.

---

### 1. Component (component.h/c)

Base type for all components. Uses C-style inheritance via struct embedding (first field).

```c
typedef enum
{
    COMPONENT_TYPE_NONE = 0,
    COMPONENT_TYPE_SCENE,
    COMPONENT_TYPE_MESH,
    COMPONENT_TYPE_MOVE,
    COMPONENT_TYPE_CAMERA,
    COMPONENT_TYPE_CAMERA_TPS,
    COMPONENT_TYPE_BOX,
    COMPONENT_TYPE_SPHERE,
    COMPONENT_TYPE_CAPSULE,         /* Phase 5 — reserved */
    COMPONENT_TYPE_MODEL,           /* Phase 7 — reserved */
    COMPONENT_TYPE_AUDIO_SOURCE,    /* Phase 8 — reserved */
    COMPONENT_TYPE_AI_CONTROLLER,   /* Phase 11 — reserved */
    COMPONENT_TYPE_FSM,             /* Phase 3 — reserved */
    NUM_COMPONENT_TYPES
} ComponentType;

typedef void (*ComponentUpdateFn)(Component* self, float deltaTime);
typedef void (*ComponentInputFn)(Component* self);
typedef void (*ComponentDestroyFn)(Component* self);

struct Component
{
    Actor*           owner;         /* Owning actor (never NULL after Init) */
    ComponentType    type;          /* Runtime type identifier for safe casting */
    int              updateOrder;   /* Lower values update first */

    ComponentUpdateFn   Update;     /* Per-tick logic (NULL = no update) */
    ComponentInputFn    Input;      /* Per-frame input (NULL = no input) */
    ComponentDestroyFn  Destroy;    /* Cleanup before free (NULL = no cleanup) */
};

/* Initialize base fields and register with owner Actor. */
void COMPONENT_Init(Component* comp, Actor* owner, ComponentType type, int updateOrder);

/* Call Destroy callback, remove from owner, free from pool. */
void COMPONENT_Destroy(Component* comp);

/* Human-readable type name for debug display. */
const char* COMPONENT_GetTypeName(ComponentType type);
```

---

### 2. SceneComponent (scene_component.h/c)

Adds transform hierarchy to Component. Parent-child relationships with dirty-flag caching.

```c
#define SCENE_MAX_CHILDREN 16

struct SceneComponent
{
    Component base;

    /* Local transform */
    Vector3 position;               /* Local position */
    Vector3 rotation;               /* Euler radians: pitch=X, yaw=Y, roll=Z */
    Vector3 scale;                  /* Per-axis scale */

    /* Cached (dirty-flag driven) */
    Matrix  localTransform;
    Matrix  worldTransform;
    bool    isDirty;

    /* Hierarchy */
    SceneComponent* parent;
    SceneComponent* children[SCENE_MAX_CHILDREN];
    int             childCount;

    /* Interpolation (for fixed-timestep rendering) */
    Vector3 prevPosition;
    Vector3 prevRotation;
    Vector3 savedPosition;          /* True physics state backup */
    Vector3 savedRotation;
};

/* Init as actor's embedded root (no component list registration). */
void SCENE_COMPONENT_InitRoot(SceneComponent* sc, Actor* owner);

/* Init as a child component (registers with actor's component list). */
void SCENE_COMPONENT_Init(SceneComponent* sc, Actor* owner,
                           ComponentType type, int updateOrder);

/* Hierarchy management. */
void SCENE_COMPONENT_AttachChild(SceneComponent* parent, SceneComponent* child);
void SCENE_COMPONENT_DetachChild(SceneComponent* parent, SceneComponent* child);
void SCENE_COMPONENT_DetachFromParent(SceneComponent* sc);

/* Dirty flag and recomputation. */
void    SCENE_COMPONENT_MarkDirty(SceneComponent* sc);
void    SCENE_COMPONENT_ComputeWorldTransform(SceneComponent* sc);

/* Direction vectors (auto-resolve dirty). */
Vector3 SCENE_COMPONENT_GetForward(SceneComponent* sc);
Vector3 SCENE_COMPONENT_GetRight(SceneComponent* sc);
Vector3 SCENE_COMPONENT_GetUp(SceneComponent* sc);
Vector3 SCENE_COMPONENT_GetWorldPosition(SceneComponent* sc);
Matrix  SCENE_COMPONENT_GetWorldTransform(SceneComponent* sc);
float   SCENE_COMPONENT_GetWorldScale(SceneComponent* sc);

/* Interpolation for fixed-timestep rendering. */
void SCENE_COMPONENT_SavePrevState(SceneComponent* sc);
void SCENE_COMPONENT_InterpolateForRender(SceneComponent* sc, float alpha);
void SCENE_COMPONENT_RestoreFromInterpolation(SceneComponent* sc);

Actor* SCENE_COMPONENT_GetOwner(SceneComponent* sc);
```

---

### 3. Actor (actor.h/c)

Entity that owns components and a root transform.

```c
#define ACTOR_MAX_COMPONENTS 16

typedef enum
{
    ACTOR_STATE_ACTIVE,
    ACTOR_STATE_PAUSED,
    ACTOR_STATE_DEAD,
} ActorState;

typedef enum
{
    ACTOR_TYPE_NONE = 0,
    ACTOR_TYPE_TPS,
    ACTOR_TYPE_ENEMY,           /* Phase 11 — reserved */
    ACTOR_TYPE_PROJECTILE,      /* Future — reserved */
    NUM_ACTOR_TYPES
} ActorType;

typedef void (*ActorUpdateFn)(Actor* self, float deltaTime);
typedef void (*ActorInputFn)(Actor* self);
typedef void (*ActorDestroyFn)(Actor* self);

struct Actor
{
    SceneComponent root;            /* Embedded transform hierarchy root */

    ActorState  state;
    ActorType   type;
    unsigned int tags;              /* Bitmask for queries and filtering */

    Game* game;                     /* Back-pointer to owning Game */

    ActorUpdateFn   Update;         /* Per-tick logic (NULL = no update) */
    ActorInputFn    Input;          /* Per-frame input (NULL = no input) */
    ActorDestroyFn  Destroy;        /* Custom cleanup (NULL = default) */

    Component* components[ACTOR_MAX_COMPONENTS];
    int        componentCount;
};

/* Allocate from pool and initialize. */
Actor* ACTOR_Create(Game* game);

/* Destroy all components, remove from game, free to pool. */
void ACTOR_Destroy(Actor* actor);

/* Update actor and all its components (called per fixed-step). */
void ACTOR_Update(Actor* actor, float deltaTime);
void ACTOR_UpdateComponents(Actor* actor, float deltaTime);

/* Process input for actor and all its components (called per frame). */
void ACTOR_ProcessInput(Actor* actor);

/* Recompute world transform for root and all children. */
void ACTOR_ComputeWorldTransform(Actor* actor);

/* Transform getters (delegates to root SceneComponent). */
Vector3 ACTOR_GetForward(Actor* actor);
Vector3 ACTOR_GetRight(Actor* actor);
Vector3 ACTOR_GetUp(Actor* actor);
Vector3 ACTOR_GetWorldPosition(Actor* actor);

/* Transform setters. */
void ACTOR_SetPosition(Actor* actor, Vector3 pos);
void ACTOR_SetRotation(Actor* actor, Vector3 euler);
void ACTOR_SetScale(Actor* actor, float uniformScale);

/* Rotate actor to face direction (yaw only via atan2 for TPS). */
void ACTOR_RotateToNewForward(Actor* actor, Vector3 forward);

/* Component management. */
void       ACTOR_AddComponent(Actor* actor, Component* comp);
void       ACTOR_RemoveComponent(Actor* actor, Component* comp);
Component* ACTOR_GetComponentOfType(Actor* actor, ComponentType type);
int        ACTOR_GetComponentsOfType(Actor* actor, ComponentType type,
                                      Component** outArray, int maxResults);
```

---

### 4. MeshComponent (mesh_component.h/c)

Renderable mesh with material, tint, and bounding box.

```c
struct MeshComponent
{
    SceneComponent scene;           /* Inherits transform + hierarchy */
    Mesh*       mesh;               /* Raylib mesh pointer */
    Material*   material;           /* Raylib material pointer */
    Color       tint;               /* Per-instance color tint */
    bool        visible;            /* Rendering toggle */
    BoundingBox localBB;            /* Computed from mesh on Create */
};

/* Allocate from pool, attach to owner, register with Renderer. */
MeshComponent* MESH_COMPONENT_Create(Actor* owner, Mesh* mesh, Material* material);

/* Draw this mesh (called by Renderer). */
void MESH_COMPONENT_Draw(MeshComponent* mc);

/* Visibility and tint control. */
void MESH_COMPONENT_SetVisible(MeshComponent* mc, bool visible);
void MESH_COMPONENT_SetTint(MeshComponent* mc, Color tint);

/* Get world-space bounding box (transformed from localBB). */
BoundingBox MESH_COMPONENT_GetWorldBB(MeshComponent* mc);
```

---

### 5. MoveComponent (move_component.h/c)

Simple linear/angular/strafe movement applied per tick.

```c
struct MoveComponent
{
    Component base;
    float angularSpeed;             /* Radians/sec around Y axis */
    float forwardSpeed;             /* Units/sec along actor forward */
    float strafeSpeed;              /* Units/sec along actor right */
};

/* Allocate from pool and attach to owner. */
MoveComponent* MOVE_COMPONENT_Create(Actor* owner);

/* One-liner speed configuration. */
void MOVE_COMPONENT_SetSpeeds(MoveComponent* mc,
                               float forward, float angular, float strafe);
```

---

### 6. BoxComponent (box_component.h/c)

AABB collider that registers with PhysWorld.

```c
struct BoxComponent
{
    Component   base;
    BoundingBox objectBox;          /* Local-space AABB */
    BoundingBox worldBox;           /* Recomputed per frame from world transform */
};

/* Allocate from pool, attach to owner, register with PhysWorld. */
BoxComponent* BOX_COMPONENT_Create(Actor* owner);

/* Set local-space AABB directly. */
void BOX_COMPONENT_SetObjectBox(BoxComponent* bc, BoundingBox box);

/* Set AABB from a mesh's bounding box. */
void BOX_COMPONENT_SetFromMesh(BoxComponent* bc, Mesh mesh);

/* Get current world-space AABB. */
BoundingBox BOX_COMPONENT_GetWorldBox(BoxComponent* bc);

/* Draw wireframe for debug visualization. */
void BOX_COMPONENT_DrawWorldBox(BoxComponent* bc, Color color);
```

---

### 7. SphereComponent (sphere_component.h/c)

Sphere collider that registers with PhysWorld.

```c
struct SphereComponent
{
    Component   base;
    Vector3     offset;             /* Center offset from actor origin (local) */
    float       radius;             /* Local-space radius */
    Vector3     worldCenter;        /* Recomputed per frame */
    float       worldRadius;        /* radius * world scale */
};

/* Allocate from pool, attach to owner, register with PhysWorld. */
SphereComponent* SPHERE_COMPONENT_Create(Actor* owner);

/* Set local offset and radius. */
void SPHERE_COMPONENT_Set(SphereComponent* sc, Vector3 offset, float radius);

/* Get world-space center and scaled radius. */
Vector3 SPHERE_COMPONENT_GetWorldCenter(SphereComponent* sc);
float   SPHERE_COMPONENT_GetWorldRadius(SphereComponent* sc);

/* Draw wireframe for debug visualization. */
void SPHERE_COMPONENT_DrawWires(SphereComponent* sc, Color color);
```

---

### 8. CameraComponent (camera_component.h/c)

Base camera wrapping Raylib's Camera3D.

```c
typedef struct
{
    SceneComponent scene;           /* Inherits transform + hierarchy */
    Camera3D  cam;                  /* Raylib camera struct */
} CameraComponent;

/* Init into pre-allocated memory (embedded in CameraTPS). */
void CAMERA_COMPONENT_Init(CameraComponent* cc, Actor* owner);

/* Push this camera to the Renderer as active camera. */
void CAMERA_COMPONENT_Apply(CameraComponent* cc);

/* Getters. */
Actor*   CAMERA_COMPONENT_GetOwner(CameraComponent* cc);
Camera3D CAMERA_COMPONENT_GetCamera(CameraComponent* cc);
```

---

### 9. CameraTPS (camera_tps.h/c)

Third-person spring camera with wall collision avoidance.

```c
typedef struct
{
    CameraComponent base;           /* Inherits camera + scene transform */

    Vector3 actualPos;              /* Current smoothed position */
    Vector3 velocity;               /* Spring velocity for damping */

    float horzDist;                 /* Horizontal distance behind target */
    float vertDist;                 /* Vertical distance above target */
    float targetDist;               /* Distance to look-at point ahead of target */
    float springConstant;           /* Spring stiffness (c = 2√k for critical damping) */
} CameraTPS;

/* Allocate from pool, attach to owner, init with defaults. */
CameraTPS* CAMERA_TPS_Create(Actor* owner);

/* Teleport camera to ideal position (skip spring). */
void CAMERA_TPS_SnapToIdeal(CameraTPS* ctps);

/* Runtime parameter adjustment. */
void CAMERA_TPS_SetDistances(CameraTPS* ctps,
                              float horz, float vert, float target);
void CAMERA_TPS_SetSpring(CameraTPS* ctps, float springConstant);
```

---

### 10. Collision Utilities (collision.h/c)

Stateless geometric functions. No system state, no registration.

```c
/* AABB operations. */
BoundingBox COLLISION_TransformAABB(BoundingBox local, Matrix transform);
BoundingBox COLLISION_MergeAABB(BoundingBox a, BoundingBox b);
BoundingBox COLLISION_ExpandAABB(BoundingBox box, Vector3 velocity);
Vector3     COLLISION_AABBCenter(BoundingBox box);
Vector3     COLLISION_AABBExtents(BoundingBox box);

/* Penetration tests (returns depth, <=0 if no overlap). */
float COLLISION_BoxVsBox(BoundingBox a, BoundingBox b, Vector3* outNormal);
float COLLISION_SphereVsSphere(Vector3 ca, float ra, Vector3 cb, float rb,
                                Vector3* outNormal);
float COLLISION_BoxVsSphere(BoundingBox box, Vector3 center, float radius,
                             Vector3* outNormal);

/* Distance queries. */
Vector3 COLLISION_ClosestPointOnSegment(Vector3 a, Vector3 b, Vector3 p);
float   COLLISION_PointToAABBDistSq(Vector3 point, BoundingBox box);
```

---

### 11. PhysWorld (physics_world.h/c)

Spatial query system and collision broad-phase.

```c
#define PHYS_WORLD_MAX_BOXES    512
#define PHYS_WORLD_MAX_SPHERES  512

typedef struct
{
    bool        hit;
    float       distance;
    Vector3     point;
    Vector3     normal;
    Component*  collider;           /* Cast via ->type to get typed component */
    Actor*      actor;
} CollisionInfo;

typedef void (*CollisionPairFn)(Actor* a, Actor* b,
                                 Vector3 normal, float penetration);

typedef struct
{
    Actor*     actorA;
    Actor*     actorB;
    Component* colliderA;
    Component* colliderB;
    Vector3    contactPoint;
    Vector3    contactNormal;       /* Points from A to B */
    float      penetration;
} ContactInfo;

typedef void (*ContactCallbackFn)(const ContactInfo* contact);

typedef struct
{
    BoxComponent*     boxes[PHYS_WORLD_MAX_BOXES];
    int               boxCount;
    SphereComponent*  spheres[PHYS_WORLD_MAX_SPHERES];
    int               sphereCount;

    ContactCallbackFn onContact;
    CollisionPairFn   onPairCollision;
} PhysWorld;

/* Lifecycle. */
void PHYS_WORLD_Init(PhysWorld* world);
void PHYS_WORLD_Shutdown(PhysWorld* world);
void PHYS_WORLD_Update(PhysWorld* world, float deltaTime);

/* Registration. */
void PHYS_WORLD_AddBox(PhysWorld* world, BoxComponent* box);
void PHYS_WORLD_RemoveBox(PhysWorld* world, BoxComponent* box);
void PHYS_WORLD_AddSphere(PhysWorld* world, SphereComponent* sphere);
void PHYS_WORLD_RemoveSphere(PhysWorld* world, SphereComponent* sphere);

/* Raycast against all colliders. Returns closest hit. */
bool PHYS_WORLD_RayCast(PhysWorld* world, Ray ray, float maxDist,
                         uint32_t layerMask, CollisionInfo* outHit);

/* Raycast skipping one actor (don't hit shooter). */
bool PHYS_WORLD_RayCastIgnore(PhysWorld* world, Ray ray, float maxDist,
                               uint32_t layerMask, Actor* ignore,
                               CollisionInfo* outHit);

/* Area overlap queries. */
int PHYS_WORLD_OverlapSphere(PhysWorld* world, Vector3 center, float radius,
                              uint32_t layerMask, Actor** outActors, int maxResults);
int PHYS_WORLD_OverlapBox(PhysWorld* world, BoundingBox box,
                           uint32_t layerMask, Actor** outActors, int maxResults);

/* Sweep (CCD). */
bool PHYS_WORLD_SphereCast(PhysWorld* world, Vector3 origin, float radius,
                            Vector3 direction, float maxDist,
                            uint32_t layerMask, CollisionInfo* outHit);

/* Broad-phase pair testing. */
void PHYS_WORLD_TestPairwise(PhysWorld* world, CollisionPairFn fn);
void PHYS_WORLD_TestSweepAndPrune(PhysWorld* world, CollisionPairFn fn);
```

---

### 12. Renderer (renderer.h/c)

Frustum culling, material-sorted draw list, frame rendering.

```c
#define RENDERER_MAX_MESHES 1024

typedef struct { Vector4 plane; } FrustumPlane;
typedef struct { MeshComponent* mc; float sortKey; } DrawEntry;

typedef struct
{
    Camera3D camera;
    Color    clearColor;

    MeshComponent* meshes[RENDERER_MAX_MESHES];
    int            meshCount;

    DrawEntry drawList[RENDERER_MAX_MESHES];
    int       drawCount;

    FrustumPlane frustum[6];

    int statsCulled;
    int statsDrawn;
} Renderer;

/* Lifecycle. */
void RENDERER_Init(Renderer* renderer);
void RENDERER_Shutdown(Renderer* renderer);
void RENDERER_DrawFrame(Renderer* renderer, Game* game);

/* Mesh registration. */
void RENDERER_AddMesh(Renderer* renderer, MeshComponent* mc);
void RENDERER_RemoveMesh(Renderer* renderer, MeshComponent* mc);

/* Camera control. */
void     RENDERER_SetCamera(Renderer* renderer, Camera3D camera);
Camera3D RENDERER_GetCamera(const Renderer* renderer);
void     RENDERER_SetClearColor(Renderer* renderer, Color color);

/* Frustum utilities (stateless, usable externally). */
void RENDERER_ExtractFrustumPlanes(FrustumPlane planes[6], Matrix viewProj);
bool RENDERER_IsAABBInFrustum(const FrustumPlane planes[6], BoundingBox box);
bool RENDERER_IsPointInFrustum(const FrustumPlane planes[6], Vector3 point);
bool RENDERER_IsSphereInFrustum(const FrustumPlane planes[6],
                                 Vector3 center, float radius);

/* World-to-screen projection (for UI markers, debug labels). */
Vector2 RENDERER_WorldToScreen(const Renderer* renderer, Vector3 worldPos);
bool    RENDERER_IsOnScreen(const Renderer* renderer, Vector3 worldPos);
```

---

### 13. Memory (memory.h/c)

Pool allocators wrapping rmem. Zero `malloc` during gameplay.

```c
typedef struct
{
    ObjPool actorPool;              /* Fixed-size: Actor structs */
    MemPool componentPool;          /* Variable-size: all Component types */
} MemorySystem;

/* Lifecycle. */
void MEMORY_Init(MemorySystem* memory);
void MEMORY_Shutdown(MemorySystem* memory);

/* Actor pool (fixed-size, O(1)). */
Actor* MEMORY_AllocActor(MemorySystem* memory);
void   MEMORY_FreeActor(MemorySystem* memory, Actor* actor);

/* Component pool (variable-size, bucketed). */
void* MEMORY_AllocComponent(MemorySystem* memory, size_t size);
void  MEMORY_FreeComponent(MemorySystem* memory, void* comp);

/* Diagnostics. */
int    MEMORY_GetActorPoolUsed(const MemorySystem* memory);
int    MEMORY_GetActorPoolTotal(const MemorySystem* memory);
size_t MEMORY_GetComponentPoolUsed(const MemorySystem* memory);
size_t MEMORY_GetComponentPoolTotal(const MemorySystem* memory);
size_t MEMORY_GetComponentPoolFree(const MemorySystem* memory);
int    MEMORY_GetComponentPoolFreeListLength(const MemorySystem* memory);
```

---

### 14. Game (game.h/c)

Application framework. Owns all subsystems, runs the main loop, manages actor lifecycle.

```c
#define SCREEN_WIDTH    1024
#define SCREEN_HEIGHT   768
#define GAME_TITLE      "Prototype Horde"
#define UPDATE_RATE     60
#define FIXED_TIMESTEP  (1.0f / (float)UPDATE_RATE)
#define MAX_DELTA_TIME  0.25f
#define RENDER_FPS      30
#define GAME_MAX_ACTORS 512
#define GAME_MAX_PENDING 64

typedef enum
{
    GAME_STATE_GAMEPLAY,
    GAME_STATE_PAUSED,
    GAME_STATE_QUIT
} GameState;

struct Game
{
    GameState state;
    float accumulator;              /* Unprocessed time for fixed-timestep */
    int updateCount;                /* Fixed-steps this frame */

    MemorySystem memory;
    LevelManager levelMgr;
    Renderer renderer;
    PhysWorld physWorld;

    Actor *actors[GAME_MAX_ACTORS];
    int    actorCount;
    Actor *pendingActors[GAME_MAX_PENDING];
    int    pendingCount;
    bool   updatingActors;
    int    actorsCreated;           /* Lifetime counter */
};

/* Initialize all subsystems and open window. */
bool GAME_Init(Game* game, Level* initialLevel);

/* Destroy all subsystems and close window. */
void GAME_Shutdown(Game* game);

/* Enter the main loop (blocks until quit). */
void GAME_Run(Game* game);

/* Register an actor (deferred if mid-update). */
void GAME_AddActor(Game* game, Actor* actor);

/* Unlink an actor from active or pending list. */
void GAME_RemoveActor(Game* game, Actor* actor);

/* Destroy every actor (level teardown). */
void GAME_RemoveAllActors(Game* game);

/* Request a level transition with default effects. */
void GAME_ChangeLevel(Game* game, Level* level);

/* Find first actor matching a tag bitmask. */
Actor* GAME_FindActorByTag(Game* game, unsigned int tag);

/* Find all actors matching a tag bitmask. */
int GAME_FindActorsByTag(Game* game, unsigned int tag,
                          Actor** outArray, int maxResults);

/* Get elapsed time since engine start (seconds). */
float GAME_GetTime(Game* game);
```

---

### 15. Level (level.h)

Level interface — static struct with function pointer callbacks.

```c
typedef void (*LevelInitFn)(Game* game);
typedef void (*LevelShutdownFn)(Game* game);
typedef void (*LevelInputFn)(Game* game);
typedef void (*LevelRender3DFn)(Game* game);
typedef int  (*LevelRenderHUDFn)(Game* game, int y);

typedef struct Level
{
    const char* name;
    LevelInitFn     Init;           /* Spawn actors, set up scene */
    LevelShutdownFn Shutdown;       /* Cleanup level resources */
    LevelInputFn    ProcessInput;   /* Level-specific input */
    LevelRender3DFn Render3D;       /* Level-specific 3D drawing */
    LevelRenderHUDFn RenderHUD;     /* Level-specific HUD overlay */
} Level;
```

---

### 16. LevelManager (level_manager.h/c)

Level lifecycle management with visual transitions.

```c
typedef enum TransitionState
{
    TRANSITION_IDLE,
    TRANSITION_FADING_OUT,
    TRANSITION_FADING_IN,
} TransitionState;

typedef void (*TransitionEffectFn)(float progress);

#define TRANSITION_DEFAULT_DURATION   0.4f
#define TRANSITION_DEFAULT_EFFECT_OUT TRANSITION_Fade
#define TRANSITION_DEFAULT_EFFECT_IN  NULL

struct LevelManager
{
    Level* activeLevel;
    Level* pendingLevel;
    TransitionState state;
    TransitionEffectFn effectOut;
    TransitionEffectFn effectIn;
    float duration;
    float progress;
};

/* Built-in transition effects. */
void TRANSITION_Fade(float progress);
void TRANSITION_WipeLeft(float progress);
void TRANSITION_WipeRight(float progress);

/* Initialize with first level (no transition). */
void LEVEL_MGR_Init(LevelManager* mgr, Game* game, Level* initialLevel);

/* Tear down current level and cleanup. */
void LEVEL_MGR_Shutdown(LevelManager* mgr, Game* game);

/* Advance transition state machine. */
void LEVEL_MGR_Update(LevelManager* mgr, Game* game, float deltaTime);

/* Draw transition effect overlay. */
void LEVEL_MGR_Render(const LevelManager* mgr);

/* Begin transition to a new level. */
void LEVEL_MGR_TransitionTo(LevelManager* mgr, Level* level,
                             TransitionEffectFn effectOut,
                             TransitionEffectFn effectIn, float duration);

/* Query transition state. */
bool        LEVEL_MGR_IsTransitioning(const LevelManager* mgr);
Level*      LEVEL_MGR_GetActiveLevel(const LevelManager* mgr);
const char* LEVEL_MGR_GetStateName(const LevelManager* mgr);
float       LEVEL_MGR_GetProgress(const LevelManager* mgr);
```

---

### 17. Debug (debug.h/c)

Debug overlay system. Currently monolithic — planned modular refactor in Phase 0.

```c
void DEBUG_Init(void);
void DEBUG_Update(Game* game);
void DEBUG_Render(Game* game);
```

---

## PLANNED MODULES

These modules are designed but not yet implemented. API shown here is the target design.

---

### Phase 0 — Debug Tool Registry (debug.h refactor)

```c
typedef struct
{
    const char* name;
    int         key;                /* Toggle hotkey (KEY_F2, etc.) */
    bool        enabled;
    void (*Update)(Game* game);
    void (*RenderOverlay)(Game* game, int* y);
    void (*Render3D)(Game* game);
} DebugTool;

void DEBUG_Init(void);
void DEBUG_Shutdown(void);
void DEBUG_RegisterTool(DebugTool tool);
void DEBUG_UnregisterTool(const char* name);
void DEBUG_Update(Game* game);
void DEBUG_Render(Game* game);
void DEBUG_Render3D(Game* game);
bool DEBUG_IsMasterVisible(void);
void DEBUG_SetMasterVisible(bool visible);
bool DEBUG_IsToolEnabled(const char* name);
```

---

### Phase 1 — InputSystem (input_system.h)

```c
typedef struct
{
    const char* name;
    int keys[4];
    int gamepadButtons[4];
    int gamepadAxis;
    float axisScale;
    float deadZone;
    bool pressed, released, held;
    float value;
} InputAction;

void  INPUT_Init(InputSystem* input);
void  INPUT_Shutdown(InputSystem* input);
void  INPUT_Update(InputSystem* input);
void  INPUT_RegisterAction(InputSystem* input, const char* name, int key0, int key1);
void  INPUT_BindGamepadButton(InputSystem* input, const char* name, int button);
void  INPUT_BindGamepadAxis(InputSystem* input, const char* name,
                             int axis, float scale, float deadZone);
bool  INPUT_IsActionPressed(const InputSystem* input, const char* name);
bool  INPUT_IsActionHeld(const InputSystem* input, const char* name);
float INPUT_GetActionValue(const InputSystem* input, const char* name);
```

---

### Phase 2 — EventSystem (event_system.h)

```c
typedef enum { EVENT_NONE, EVENT_ACTOR_SPAWNED, EVENT_ACTOR_DESTROYED,
               EVENT_ACTOR_DAMAGED, EVENT_ACTOR_DIED, EVENT_COLLISION_ENTER,
               EVENT_COLLISION_EXIT, EVENT_TRIGGER_ENTER, EVENT_TRIGGER_EXIT,
               EVENT_HEALTH_CHANGED, EVENT_AMMO_CHANGED, NUM_EVENT_TYPES } EventType;

void EVENT_Init(EventSystem* sys);
void EVENT_Shutdown(EventSystem* sys);
void EVENT_Subscribe(EventSystem* sys, EventType type, EventCallbackFn fn, void* userData);
void EVENT_Unsubscribe(EventSystem* sys, EventType type, EventCallbackFn fn);
void EVENT_Fire(EventSystem* sys, Event event);       /* Immediate dispatch */
void EVENT_Post(EventSystem* sys, Event event);       /* Deferred dispatch */
void EVENT_ProcessQueue(EventSystem* sys);
```

---

### Phase 3 — FSM (fsm.h)

```c
void FSM_Init(FSM* fsm);
void FSM_Update(FSM* fsm, Actor* actor, float deltaTime);
int  FSM_AddState(FSM* fsm, State* state);
void FSM_ChangeState(FSM* fsm, Actor* actor, int stateIndex);
void FSM_ChangeStateByName(FSM* fsm, Actor* actor, const char* name);
void FSM_RevertToPrevious(FSM* fsm, Actor* actor);
const char* FSM_GetCurrentStateName(const FSM* fsm);
FSMComponent* FSM_COMPONENT_Create(Actor* owner);
```

---

### Phase 4 — CharacterMovement + PlayerController

```c
CharacterMovement* CHARACTER_Create(Actor* owner, CapsuleComponent* capsule);
void CHARACTER_Move(CharacterMovement* cm, Vector3 worldDirection, float speed);
void CHARACTER_Jump(CharacterMovement* cm);
bool CHARACTER_IsGrounded(const CharacterMovement* cm);

PlayerController* PLAYER_CONTROLLER_Create(Actor* owner,
    CharacterMovement* movement, CameraTPS* camera, FSMComponent* fsm);
```

---

### Phase 5 — CapsuleComponent + Collision Layers + CCD

```c
CapsuleComponent* CAPSULE_COMPONENT_Create(Actor* owner);
void CAPSULE_COMPONENT_Set(CapsuleComponent* cc, float height, float radius, Vector3 offset);
/* New collision tests: CapsuleVsBox, CapsuleVsSphere, CapsuleVsCapsule */
/* Collision layers: 32-bit masks, layer collision matrix */
/* CCD: capsule sweep, sphere sweep, moving AABB sweep */
```

---

### Phase 5B — Prefab System

```c
void          PREFAB_RegistryInit(void);
void          PREFAB_RegistryShutdown(void);
bool          PREFAB_Register(const Prefab* prefab);
const Prefab* PREFAB_Find(const char* name);
Actor*        PREFAB_Spawn(Game* game, const Prefab* prefab, Vector3 position, Vector3 rotation);
Actor*        PREFAB_Instantiate(Game* game, const char* name, Vector3 position, Vector3 rotation);
```

---

### Phase 6 — AssetManager

```c
void        ASSET_Init(AssetManager* mgr);
void        ASSET_Shutdown(AssetManager* mgr);
AssetHandle ASSET_LoadMesh(AssetManager* mgr, const char* name, const char* path);
AssetHandle ASSET_RegisterMesh(AssetManager* mgr, const char* name, Mesh mesh);
Mesh*       ASSET_GetMesh(AssetManager* mgr, const char* name);
void        ASSET_Acquire(AssetManager* mgr, AssetHandle handle);
void        ASSET_Release(AssetManager* mgr, AssetHandle handle);
```

---

### Phase 7-12 — See PLAN.md

Animation, Audio, Serialization, Rendering improvements, AI, and UI systems are designed in the development plan. Their API signatures are documented in the original `api_reference.md` project file.

---

## Cross-Module Integration

```
GAME_Init order:     Memory → Window → Renderer → PhysWorld → Debug → LevelManager
GAME_Shutdown order: LevelManager → Renderer → Window → Memory (reverse)

Component registration:
  MeshComponent  → RENDERER_AddMesh / RemoveMesh
  BoxComponent   → PHYS_WORLD_AddBox / RemoveBox
  SphereComponent → PHYS_WORLD_AddSphere / RemoveSphere
  CameraComponent → RENDERER_SetCamera (via Apply)

Actor lifecycle:
  GAME_AddActor → actors[] or pendingActors[]
  FixedUpdate   → Update actors → Flush pending → Destroy dead
  GAME_RemoveActor → O(1) swap-remove
```