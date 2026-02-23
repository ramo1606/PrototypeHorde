#include "actor.h"
#include "component.h"
#include "game.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

static void ACTOR_Init(Actor* actor, Game* game);

Actor* ACTOR_Create(Game* game)
{
    assert(game != NULL);

    Actor* actor = MEMORY_AllocActor(&game->memory);
    if (!actor) return NULL;

    ACTOR_Init(actor, game);
    return actor;
}

void ACTOR_Init(Actor* actor, Game* game) 
{
	assert(actor != NULL);
	assert(game != NULL);

    SCENE_COMPONENT_InitRoot(&actor->root, actor);

    actor->state = ACTOR_STATE_ACTIVE;
    actor->type  = ACTOR_NONE;
    actor->game  = game;

    actor->Update = NULL;
    actor->Input = NULL;
    actor->Destroy = NULL;

    actor->componentCount = 0;
    memset(actor->components, 0, sizeof(actor->components));

	GAME_AddActor(game, actor);
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

	Game* game = actor->game;
	GAME_RemoveActor(game, actor);
    MEMORY_FreeActor(&game->memory, actor);
}

void ACTOR_Update(Actor* actor, float deltaTime) 
{
	assert(actor != NULL);

    if (actor->state != ACTOR_STATE_ACTIVE) return;

    /* Resolve root transform before components read it */
    ACTOR_ComputeWorldTransform(actor);

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
    SCENE_COMPONENT_ComputeWorldTransform(&actor->root);
}

//TODO: revisar que el sistema de coordenadas sea correcto, podría ser que el forward sea Z+ y no X+
Vector3 ACTOR_GetForward(Actor* actor) 
{
	assert(actor != NULL);
    return SCENE_COMPONENT_GetForward(&actor->root);
}

Vector3 ACTOR_GetRight(Actor* actor) 
{
	assert(actor != NULL);
    return SCENE_COMPONENT_GetRight(&actor->root);
}

Vector3 ACTOR_GetUp(Actor* actor)
{
	assert(actor != NULL);
    return SCENE_COMPONENT_GetUp(&actor->root);
}

Vector3 ACTOR_GetWorldPosition(Actor* actor)
{
    assert(actor != NULL);
    return SCENE_COMPONENT_GetWorldPosition(&actor->root);
}

void ACTOR_SetPosition(Actor* actor, Vector3 pos) 
{
	assert(actor != NULL);

    actor->root.position = pos;
    SCENE_COMPONENT_MarkDirty(&actor->root);
}

void ACTOR_SetRotation(Actor* actor, Vector3 euler) 
{
    assert(actor != NULL);
    actor->root.rotation = euler;
    SCENE_COMPONENT_MarkDirty(&actor->root);
}

void ACTOR_SetScale(Actor* actor, float scale) 
{
	assert(scale > 0.0f);
	assert(actor != NULL);

    actor->root.scale = (Vector3){ scale, scale, scale };
    SCENE_COMPONENT_MarkDirty(&actor->root);
}

void ACTOR_RotateToNewForward(Actor* actor, Vector3 forward) 
{
    assert(actor != NULL);

    /* Project onto XZ plane and normalize (ignore Y component) */
    Vector3 flatFwd = { forward.x, 0.0f, forward.z };
    float len = Vector3Length(flatFwd);
    if (len < 0.0001f) return;   /* Degenerate — pointing straight up/down */

    flatFwd = Vector3Scale(flatFwd, 1.0f / len);

    float yaw = atan2f(-flatFwd.x, -flatFwd.z);
    ACTOR_SetRotation(actor, (Vector3) { 0.0f, yaw, 0.0f });
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