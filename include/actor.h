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
    ACTOR_STATE_ACTIVE,
    ACTOR_STATE_PAUSED,
    ACTOR_STATE_DEAD,
} ActorState;

typedef enum
{
    ACTOR_NONE,
    TPS_ACTOR,
    NUM_ACTOR_TYPES
} ActorType;

typedef void (*ActorUpdateFn)(Actor* self, float deltaTime);
typedef void (*ActorInputFn)(Actor* self);
typedef void (*ActorDestroyFn)(Actor* self);

struct Actor 
{
    /* Root Scene Component */
    SceneComponent root;

    /* State */
    ActorState state;
    ActorType type;

    /* Game */
    Game* game;

    /* Virtual Functions */
    ActorUpdateFn Update;
    ActorInputFn Input;
    ActorDestroyFn Destroy;

    /* Components List */
    Component* components[ACTOR_MAX_COMPONENTS];
    int componentCount;
};

/* Life Cycle */
Actor* ACTOR_Create(Game* game);
void ACTOR_Destroy(Actor* actor);

/* Update */
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

void ACTOR_RotateToNewForward(Actor* actor, Vector3 forward);

void ACTOR_AddComponent(Actor* actor, Component* comp);
void ACTOR_RemoveComponent(Actor* actor, Component* comp);
Component *ACTOR_GetComponentOfType(Actor* actor, ComponentType type);