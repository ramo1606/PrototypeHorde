#include "actor.h"
#include "game.h"
#include <stdlib.h>
#include <math.h>

void ACTOR_Init(Actor *actor, Game *game) 
{
    actor->position = (Vector3){ 0.0f, 0.0f, 0.0f };
    actor->rotation = QuaternionIdentity();
    actor->scale = 1.0f;
    actor->worldTransform = MatrixIdentity();
    actor->isDirty = true;

    actor->state = ACTOR_STATE_ACTIVE;
    actor->game  = game;

    actor->onUpdate  = NULL;
    actor->onInput   = NULL;
    actor->onDestroy = NULL;

    GAME_AddActor(game, actor);
}

void ACTOR_Destroy(Actor *actor) 
{
    if (!actor) return;

    /* Step 3: destroy all components here */

    if (actor->onDestroy) 
    {
        actor->onDestroy(actor);
    }

    free(actor);
}

void ACTOR_Update(Actor *actor, float deltaTime) 
{
    if (!actor) return;

    if (actor->state != ACTOR_STATE_ACTIVE) return;

    if (actor->isDirty) 
    {
        ACTOR_ComputeWorldTransform(actor);
    }

    /* Step 3: update components here */

    if (actor->onUpdate) 
    {
        actor->onUpdate(actor, deltaTime);
    }
}

void ACTOR_ProcessInput(Actor *actor) 
{
    if (!actor) return;
    if (actor->state != ACTOR_STATE_ACTIVE) return;

    /* Step 3: process input for components here */

    if (actor->onInput) 
    {
        actor->onInput(actor);
    }
}

void ACTOR_ComputeWorldTransform(Actor *actor) 
{
    actor->isDirty = false;

    /* SRT: Scale → Rotate → Translate */
    Matrix s = MatrixScale(actor->scale, actor->scale, actor->scale);
    Matrix r = QuaternionToMatrix(actor->rotation);
    Matrix t = MatrixTranslate(actor->position.x, actor->position.y, actor->position.z);

    actor->worldTransform = MatrixMultiply(MatrixMultiply(s, r), t);

    /* Step 3: notify components of transform change */
}

Vector3 ACTOR_GetForward(const Actor *actor) 
{
    return Vector3RotateByQuaternion((Vector3){ 1.0f, 0.0f, 0.0f }, actor->rotation);
}

Vector3 ACTOR_GetRight(const Actor *actor) 
{
    return Vector3RotateByQuaternion((Vector3){ 0.0f, 0.0f, 1.0f }, actor->rotation);
}

Vector3 ACTOR_GetUp(const Actor *actor) 
{
    return Vector3RotateByQuaternion((Vector3){ 0.0f, 1.0f, 0.0f }, actor->rotation);
}

void ACTOR_SetPosition(Actor *actor, Vector3 pos) 
{
    actor->position = pos;
    actor->isDirty = true;
}

void ACTOR_SetRotation(Actor *actor, Quaternion rot) 
{
    actor->rotation = rot;
    actor->isDirty = true;
}

void ACTOR_SetScale(Actor *actor, float scale) 
{
    actor->scale = scale;
    actor->isDirty = true;
}

void ACTOR_RotateToNewForward(Actor *actor, Vector3 forward) 
{
    float dot = Vector3DotProduct((Vector3){ 1.0f, 0.0f, 0.0f }, forward);

    if (dot > 0.9999f) 
    {
        /* Already facing forward, identity */
        ACTOR_SetRotation(actor, QuaternionIdentity());
    }
    else if (dot < -0.9999f) 
    {
        /* Facing opposite: rotate 180° around Y (up axis) */
        ACTOR_SetRotation(actor,
            QuaternionFromAxisAngle((Vector3){ 0.0f, 1.0f, 0.0f }, PI));
    }
    else 
    {
        Vector3 axis = Vector3CrossProduct(
            (Vector3){ 1.0f, 0.0f, 0.0f }, forward);
        axis = Vector3Normalize(axis);
        float angle = acosf(dot);
        ACTOR_SetRotation(actor, QuaternionFromAxisAngle(axis, angle));
    }
}