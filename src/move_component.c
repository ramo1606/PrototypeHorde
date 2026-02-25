#include "move_component.h"
#include "actor.h"
#include "game.h"
#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <assert.h>

#define NEAR_ZERO 0.001f  /* Speed threshold below which movement is skipped to avoid FP noise */

static void MoveUpdate(Component* self, float deltaTime) 
{
    /*
     * Apply angular and translational velocities to the owner actor each
     * fixed timestep.
     *
     * Angular: rotate around the world Y axis by angularSpeed * dt.
     * Forward/Strafe: translate along the actor's current world-space
     *   forward and right vectors scaled by the respective speed and dt.
     *
     * The NEAR_ZERO guard prevents accumulation of floating-point drift
     * when speeds are effectively zero.
     */
	assert(self != NULL);
    if(self->type != COMPONENT_TYPE_MOVE) 
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
    /*
     * Allocate from the component pool, register with the owner at
     * updateOrder 10 (runs early so the actor position is set before
     * camera and collider components run at orders 250 and 300).
     */
	assert(owner != NULL);
    MoveComponent* mc = (MoveComponent*)MEMORY_AllocComponent(&owner->game->memory, sizeof(MoveComponent));
    if (!mc) return NULL;

    COMPONENT_Init(&mc->base, owner, COMPONENT_TYPE_MOVE, 10);
    mc->base.Update = MoveUpdate;

    mc->angularSpeed = 0.0f;
    mc->forwardSpeed = 0.0f;
    mc->strafeSpeed = 0.0f;

    return mc;
}

void MOVE_COMPONENT_SetSpeeds(MoveComponent* mc, float forward, float angular, float strafe)
{
    assert(mc != NULL);
    mc->forwardSpeed = forward;
    mc->angularSpeed = angular;
    mc->strafeSpeed = strafe;
}