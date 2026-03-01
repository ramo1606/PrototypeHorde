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
    ACTOR_TYPE_NONE = 0,
    ACTOR_TYPE_TPS,
    ACTOR_TYPE_ENEMY,           /* Phase 11 */
    ACTOR_TYPE_PROJECTILE,      /* Future */
    NUM_ACTOR_TYPES
} ActorType;

typedef void (*ActorUpdateFn)(Actor* self, float deltaTime);
typedef void (*ActorInputFn)(Actor* self);
typedef void (*ActorDestroyFn)(Actor* self);

struct Actor 
{
    SceneComponent root;

    ActorState state;
    ActorType type;
    unsigned int tags;

    Game* game;

    ActorUpdateFn Update;
    ActorInputFn Input;
    ActorDestroyFn Destroy;

    Component* components[ACTOR_MAX_COMPONENTS];
    int componentCount;
};

Actor* ACTOR_Create(Game* game);
void ACTOR_Destroy(Actor* actor);

void ACTOR_Update(Actor* actor, float deltaTime);
void ACTOR_UpdateComponents(Actor* actor, float deltaTime);
void ACTOR_ProcessInput(Actor* actor);
void ACTOR_ComputeWorldTransform(Actor* actor);

Vector3 ACTOR_GetForward(Actor* actor);
Vector3 ACTOR_GetRight(Actor* actor);
Vector3 ACTOR_GetUp(Actor* actor);
Vector3 ACTOR_GetWorldPosition(Actor* actor);

void ACTOR_SetPosition(Actor* actor, Vector3 pos);
void ACTOR_SetRotation(Actor* actor, Vector3 euler);
void ACTOR_SetScale(Actor* actor, float scale);

// TODO: should be moved to CharacterMovementComponent?
void ACTOR_RotateToNewForward(Actor* actor, Vector3 forward);

void ACTOR_AddComponent(Actor* actor, Component* comp);
void ACTOR_RemoveComponent(Actor* actor, Component* comp);
Component *ACTOR_GetComponentOfType(Actor* actor, ComponentType type);
int ACTOR_GetComponentsOfType(Actor* actor, ComponentType type,
                               Component** outArray, int maxResults);

bool ACTOR_HasTag(Actor* actor, unsigned int tag);