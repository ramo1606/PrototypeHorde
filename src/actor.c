#include "actor.h"
#include "component.h"
#include "game.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

static const char* ActorTypeNames[NUM_ACTOR_TYPES] = 
{
    "Actor",
	"TPSActor"
};

void ACTOR_Init(Actor* actor, Game* game) 
{
	assert(actor != NULL);
	assert(game != NULL);

    actor->state = ACTOR_STATE_ACTIVE;

    actor->position = (Vector3){ 0.0f, 0.0f, 0.0f };
    actor->rotation = QuaternionIdentity();
    actor->scale = 1.0f;

    actor->worldTransform = MatrixIdentity();
    actor->isDirty = true;

    actor->Update = NULL;
    actor->Input = NULL;
    actor->Destroy = NULL;

    actor->componentCount = 0;
    memset(actor->components, 0, sizeof(actor->components));

	GAME_AddActor(game, actor);

    actor->game  = game;
}

void ACTOR_Destroy(Actor* actor) 
{
	assert(actor != NULL);

    while (actor->componentCount > 0) 
    {
        int lastIndex = actor->componentCount - 1;
		Component* comp = actor->components[lastIndex];
        COMPONENT_Destroy(comp);
    }

    if (actor->Destroy)
    {
        actor->Destroy(actor);
    }

	GAME_RemoveActor(actor->game, actor);
	free(actor);
}

void ACTOR_Update(Actor* actor, float deltaTime) 
{
	assert(actor != NULL);

    if (actor->state != ACTOR_STATE_ACTIVE) return;

    if (actor->isDirty) 
    {
        ACTOR_ComputeWorldTransform(actor);
    }

	ACTOR_UpdateComponents(actor, deltaTime);
    
    if (actor->Update)
    {
        actor->Update(actor, deltaTime);
    }
}

void ACTOR_UpdateComponents(Actor* actor, float deltaTime)
{
	assert(actor != NULL);

    for (int i = 0; i < actor->componentCount; i++)
    {
        if (actor->components[i]->Update)
        {
            actor->components[i]->Update(actor->components[i], deltaTime);
        }
    }
}

void ACTOR_ProcessInput(Actor* actor) 
{
	assert(actor != NULL);

    if (actor->state != ACTOR_STATE_ACTIVE) return;

    for (int i = 0; i < actor->componentCount; i++) 
    {
        if (actor->components[i]->Input) 
        {
            actor->components[i]->Input(actor->components[i]);
        }
    }

    if (actor->Input) 
    {
        actor->Input(actor);
    }
}

void ACTOR_ComputeWorldTransform(Actor* actor) 
{
	assert(actor != NULL);

    actor->isDirty = false;

    /* SRT: Scale → Rotate → Translate */
    Matrix s = MatrixScale(actor->scale, actor->scale, actor->scale);
    Matrix r = QuaternionToMatrix(actor->rotation);
    Matrix t = MatrixTranslate(actor->position.x, actor->position.y, actor->position.z);

    actor->worldTransform = MatrixMultiply(MatrixMultiply(s, r), t);

    for (int i = 0; i < actor->componentCount; i++) 
    {
        if (actor->components[i]->WorldTransform) 
        {
            actor->components[i]->WorldTransform(actor->components[i]);
        }
    }
}

//TODO: revisar que el sistema de coordenadas sea correcto, podría ser que el forward sea Z+ y no X+
Vector3 ACTOR_GetForward(const Actor* actor) 
{
	assert(actor != NULL);
    return Vector3RotateByQuaternion((Vector3){ 1.0f, 0.0f, 0.0f }, actor->rotation);
}

Vector3 ACTOR_GetRight(const Actor* actor) 
{
	assert(actor != NULL);
    return Vector3RotateByQuaternion((Vector3){ 0.0f, 0.0f, 1.0f }, actor->rotation);
}

Vector3 ACTOR_GetUp(const Actor* actor)
{
	assert(actor != NULL);
    return Vector3RotateByQuaternion((Vector3){ 0.0f, 1.0f, 0.0f }, actor->rotation);
}

void ACTOR_SetPosition(Actor* actor, Vector3 pos) 
{
	assert(actor != NULL);

    actor->position = pos;
    actor->isDirty = true;
}

void ACTOR_SetRotation(Actor* actor, Quaternion rot) 
{
	assert(actor != NULL);

    actor->rotation = rot;
    actor->isDirty = true;
}

void ACTOR_SetScale(Actor* actor, float scale) 
{
	assert(scale > 0.0f);
	assert(actor != NULL);

    actor->scale = scale;
    actor->isDirty = true;
}

void ACTOR_RotateToNewForward(Actor* actor, Vector3 forward) 
{
    assert(actor != NULL);

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

void ACTOR_AddComponent(Actor* actor, Component* comp) 
{
    assert(actor != NULL);
    assert(comp != NULL);

    if (actor->componentCount >= ACTOR_MAX_COMPONENTS) 
    {
        TraceLog(LOG_WARNING, "ACTOR: Component list full (%d)", ACTOR_MAX_COMPONENTS);
        return;
    }

    int insertIdx = actor->componentCount;
    for (int i = 0; i < actor->componentCount; i++) 
    {
        if (comp->updateOrder < actor->components[i]->updateOrder) 
        {
            insertIdx = i;
            break;
        }
    }

    for (int i = actor->componentCount; i > insertIdx; i--) 
    {
        actor->components[i] = actor->components[i - 1];
    }

    actor->components[insertIdx] = comp;
    actor->componentCount++;
}

void ACTOR_RemoveComponent(Actor* actor, Component* comp) 
{
    assert(actor != NULL);
    assert(comp != NULL);

    for (int i = 0; i < actor->componentCount; i++) 
    {
        if (actor->components[i] == comp) 
        {
            for (int j = i; j < actor->componentCount - 1; j++) 
            {
                actor->components[j] = actor->components[j + 1];
            }
            actor->components[actor->componentCount - 1] = NULL;
            actor->componentCount--;
            return;
        }
    }
}

Component *ACTOR_GetComponentOfType(Actor* actor, ComponentType type) 
{
    assert(actor != NULL);

    for (int i = 0; i < actor->componentCount; i++) 
    {
        if (actor->components[i]->type == type) 
        {
            return actor->components[i];
        }
    }
    return NULL;
}