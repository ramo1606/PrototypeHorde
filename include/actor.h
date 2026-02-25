#pragma once

/*
 * actor.h — The Actor: the central entity in the game world.
 *
 * An Actor is the atomic "game object".  It owns a SceneComponent root that
 * anchors it in the scene graph, a flat sorted list of attached Components,
 * and optional per-frame callbacks (Update, Input, Destroy).
 *
 * Key design decisions:
 *   - Pending actor queue: actors created during the update loop are
 *     buffered in pendingActors[] and promoted to actors[] at the end of
 *     the fixed update, avoiding invalidation of the iteration range.
 *   - Components are kept sorted by updateOrder using insertion sort so
 *     execution order is deterministic and configurable.
 *   - Actors are allocated from an ObjPool (fixed-size slab allocator) in
 *     MemorySystem for O(1) alloc/free with no heap fragmentation.
 *
 * Architecture position:
 *   Game.actors[] → Actor → SceneComponent root + Component[]
 */

#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>
#include "component.h"
#include "scene_component.h"

#define ACTOR_MAX_COMPONENTS 16  /* Maximum number of components that can be attached to one actor */

typedef struct Game Game;
typedef struct Actor Actor;

/* ── Actor State & Type Enums ───────────────────────────────────── */

typedef enum 
{
    ACTOR_STATE_ACTIVE,  /* Actor receives updates and input every tick */
    ACTOR_STATE_PAUSED,  /* Actor is frozen — no updates, no input */
    ACTOR_STATE_DEAD,    /* Actor will be destroyed at the end of the current fixed update */
} ActorState;

typedef enum
{
    ACTOR_TYPE_NONE = 0,            /* Generic actor with no special game logic */
    ACTOR_TYPE_TPS,                 /* Third-person camera-controlled player character */
    ACTOR_TYPE_ENEMY,               /* Phase 11 */
    ACTOR_TYPE_PROJECTILE,          /* Future */
    NUM_ACTOR_TYPES                 /* Sentinel — total number of actor types */
} ActorType;

/* ── Per-frame Callbacks ────────────────────────────────────────── */

typedef void (*ActorUpdateFn)(Actor* self, float deltaTime);  /* Custom gameplay update called after component updates */
typedef void (*ActorInputFn)(Actor* self);                     /* Custom input handler called once per frame */
typedef void (*ActorDestroyFn)(Actor* self);                   /* Custom teardown called before actor memory is freed */

/* ── Actor Struct ───────────────────────────────────────────────── */

struct Actor 
{
    SceneComponent root;                    /* Transform hierarchy root for this actor */

    ActorState     state;                   /* Current lifecycle state (active/paused/dead) */
    ActorType      type;                    /* Gameplay type identifier */
    unsigned int   tags;                    /* Bitmask for fast tag-based queries */

    Game*          game;                    /* Back-pointer to the owning Game instance */

    ActorUpdateFn  Update;                  /* Per-frame custom update callback (optional) */
    ActorInputFn   Input;                   /* Per-frame input processing callback (optional) */
    ActorDestroyFn Destroy;                 /* Custom cleanup callback invoked before deallocation (optional) */

    Component* components[ACTOR_MAX_COMPONENTS]; /* Flat array of attached components, sorted by updateOrder */
    int        componentCount;              /* Number of currently attached components */
};

/* ── Lifecycle ──────────────────────────────────────────────────── */

Actor* ACTOR_Create(Game* game);   // Allocate an actor from the pool, initialise it, and add it to the game world
void   ACTOR_Destroy(Actor* actor); // Destroy all components, invoke Destroy callback, unregister from game, and free memory

/* ── Per-frame ──────────────────────────────────────────────────── */

void ACTOR_Update(Actor* actor, float deltaTime);           // Recompute world transform, update all components, then invoke actor's Update callback
void ACTOR_UpdateComponents(Actor* actor, float deltaTime); // Call Update on every attached component in sorted order
void ACTOR_ProcessInput(Actor* actor);                      // Call Input on every component then invoke actor's Input callback
void ACTOR_ComputeWorldTransform(Actor* actor);             // Trigger world-transform recomputation on the root SceneComponent

/* ── Transform Accessors ────────────────────────────────────────── */

Vector3 ACTOR_GetForward(Actor* actor);        // Return the world-space forward direction of the actor
Vector3 ACTOR_GetRight(Actor* actor);          // Return the world-space right direction of the actor
Vector3 ACTOR_GetUp(Actor* actor);             // Return the world-space up direction of the actor
Vector3 ACTOR_GetWorldPosition(Actor* actor);  // Return the current world-space position of the actor

void ACTOR_SetPosition(Actor* actor, Vector3 pos);    // Set the local position and mark the root dirty
void ACTOR_SetRotation(Actor* actor, Vector3 euler);  // Set the local Euler rotation (radians) and mark the root dirty
void ACTOR_SetScale(Actor* actor, float scale);       // Set uniform scale and mark the root dirty

// TODO: should be moved to CharacterMovementComponent?
void ACTOR_RotateToNewForward(Actor* actor, Vector3 forward); // Rotate the actor to face a new forward vector (yaw only, projected onto XZ plane)

/* ── Component Management ───────────────────────────────────────── */

void       ACTOR_AddComponent(Actor* actor, Component* comp);                                                         // Insert a component into the sorted array using insertion sort on updateOrder
void       ACTOR_RemoveComponent(Actor* actor, Component* comp);                                                      // Remove a component from the array and compact the list
Component* ACTOR_GetComponentOfType(Actor* actor, ComponentType type);                                                // Return the first attached component matching the given type, or NULL
int        ACTOR_GetComponentsOfType(Actor* actor, ComponentType type, Component** outArray, int maxResults);         // Fill outArray with all components matching type; returns count found

/* ── Tag Queries ────────────────────────────────────────────────── */

bool ACTOR_HasTag(Actor* actor, unsigned int tag); // Return true if any bit of tag is set in the actor's tag bitmask