#include "move_component.h"
#include "actor.h"
#include "game.h"
#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <assert.h>

#define NEAR_ZERO 0.001f

static void MoveUpdate(Component* self, float deltaTime) 
{
	assert(self != NULL);
    if(self->type != COMPONENT_MOVE) 
    {
        return;
	}

    MoveComponent* mc = (MoveComponent*)self;
    Actor* owner = self->owner;

    if (fabsf(mc->angularSpeed) > NEAR_ZERO) 
    {
        float angle = mc->angularSpeed * deltaTime;
        Vector3 rot = owner->root.rotation;
        rot.y += angle;
        ACTOR_SetRotation(owner, rot);
    }

    if (fabsf(mc->forwardSpeed) > NEAR_ZERO ||
        fabsf(mc->strafeSpeed) > NEAR_ZERO) 
    {
        Vector3 pos = owner->root.position;
        Vector3 fwd = ACTOR_GetForward(owner);
        Vector3 right = ACTOR_GetRight(owner);

        pos = Vector3Add(pos, Vector3Scale(fwd, mc->forwardSpeed * deltaTime));
        pos = Vector3Add(pos, Vector3Scale(right, mc->strafeSpeed * deltaTime));

        ACTOR_SetPosition(owner, pos);
    }
}

MoveComponent* MOVE_COMPONENT_Create(Actor* owner)
{
	assert(owner != NULL);
    MoveComponent* mc = (MoveComponent*)MEMORY_AllocComponent(&owner->game->memory, sizeof(MoveComponent));
    if (!mc) return NULL;

    COMPONENT_Init(&mc->base, owner, COMPONENT_MOVE, 10);
    mc->base.Update = MoveUpdate;

    mc->angularSpeed = 0.0f;
    mc->forwardSpeed = 0.0f;
    mc->strafeSpeed = 0.0f;

    return mc;
}

void MOVE_COMPONENT_SetSpeeds(MoveComponent* mc, float forward, float angular, float strafe)
{
    
}