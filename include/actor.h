/*******************************************************************************************
*
*   actor.h — Actor (Game Entity)
*
*   An Actor is the fundamental game entity — anything that exists in the world (player,
*   enemies, props, cameras, triggers). Actors own a list of Components that define their
*   behavior, and a root SceneComponent that places them in the scene hierarchy.
*
*   Architecture:
*       Game ──manages──▶ Actor[]
*                            │
*                            ├── root (SceneComponent) — position, rotation, scale
*                            └── components[] — sorted by updateOrder
*
*   Lifecycle:
*       ACTOR_Create()  → Allocates from pool, calls Init, registers with Game
*       ACTOR_Update()  → Recomputes transform, updates components, calls custom Update
*       ACTOR_Destroy() → Destroys all components, calls custom Destroy, frees to pool
*
*   Double-Buffer Pattern:
*       Actors created during the update loop go into a pending list and are promoted
*       to the active list at the end of the frame. This prevents iterator invalidation.
*
*   Naming Convention:
*       Enums:   ACTOR_STATE_*, ACTOR_TYPE_*
*       API:     ACTOR_*
*
********************************************************************************************/
#pragma once

#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>
#include "component.h"
#include "scene_component.h"

#define ACTOR_MAX_COMPONENTS 16

typedef struct Game Game;
typedef struct Actor Actor;

typedef enum 
{
    ACTOR_STATE_ACTIVE,             /* Updated and rendered normally                  */
    ACTOR_STATE_PAUSED,             /* Skipped during update (still rendered)         */
    ACTOR_STATE_DEAD,               /* Marked for destruction at end of frame         */
} ActorState;

typedef enum
{
    ACTOR_TYPE_NONE = 0,            /* Generic / untyped actor                       */
    ACTOR_TYPE_TPS,                 /* Third-person player character                 */
    ACTOR_TYPE_ENEMY,               /* Phase 11: Enemy AI actor                      */
    ACTOR_TYPE_PROJECTILE,          /* Future: Bullet / projectile                   */
    NUM_ACTOR_TYPES                 /* Sentinel — total count of actor types         */
} ActorType;

/* ── Virtual Function Pointers ───────────────────────────────────────────── */
typedef void (*ActorUpdateFn)(Actor* self, float deltaTime);
typedef void (*ActorInputFn)(Actor* self);
typedef void (*ActorDestroyFn)(Actor* self);

struct Actor 
{
    SceneComponent root;            /* Embedded root transform — NOT a pointer        */

    ActorState state;               /* Current lifecycle state                        */
    ActorType  type;                /* Runtime type for gameplay queries              */
    unsigned int tags;              /* Bitmask for tagging (e.g. TAG_PLAYER = 0x01)   */

    Game* game;                     /* Back-reference to the owning Game instance     */

    ActorUpdateFn  Update;          /* Custom per-tick logic (NULL = none)            */
    ActorInputFn   Input;           /* Custom input handler (NULL = none)             */
    ActorDestroyFn Destroy;         /* Custom cleanup (NULL = none)                   */

    Component* components[ACTOR_MAX_COMPONENTS]; /* Sorted by updateOrder             */
    int        componentCount;      /* Current number of attached components          */
};

/* ── Lifecycle ───────────────────────────────────────────────────────────── */
Actor* ACTOR_Create(Game* game);
void ACTOR_Destroy(Actor* actor);

/* ── Update Loop ─────────────────────────────────────────────────────────── */
void ACTOR_Update(Actor* actor, float deltaTime);
void ACTOR_UpdateComponents(Actor* actor, float deltaTime);
void ACTOR_ProcessInput(Actor* actor);
void ACTOR_ComputeWorldTransform(Actor* actor);

/* ── Transform Getters ───────────────────────────────────────────────────── */
Vector3 ACTOR_GetForward(Actor* actor);
Vector3 ACTOR_GetRight(Actor* actor);
Vector3 ACTOR_GetUp(Actor* actor);
Vector3 ACTOR_GetWorldPosition(Actor* actor);

/* ── Transform Setters ───────────────────────────────────────────────────── */
void ACTOR_SetPosition(Actor* actor, Vector3 pos);
void ACTOR_SetRotation(Actor* actor, Vector3 euler);
void ACTOR_SetScale(Actor* actor, float scale);

// TODO: should be moved to CharacterMovementComponent?
void ACTOR_RotateToNewForward(Actor* actor, Vector3 forward);

/* ── Component Management ────────────────────────────────────────────────── */
void ACTOR_AddComponent(Actor* actor, Component* comp);
void ACTOR_RemoveComponent(Actor* actor, Component* comp);
Component *ACTOR_GetComponentOfType(Actor* actor, ComponentType type);
int ACTOR_GetComponentsOfType(Actor* actor, ComponentType type,
                               Component** outArray, int maxResults);

/* ── Tag Queries ─────────────────────────────────────────────────────────── */
bool ACTOR_HasTag(Actor* actor, unsigned int tag);