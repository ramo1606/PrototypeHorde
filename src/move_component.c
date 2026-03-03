/*******************************************************************************************
*
*   move_component.c — Move Component Implementation
*
********************************************************************************************/
#include "move_component.h"
#include "actor.h"
#include "game.h"
#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <assert.h>

/* Threshold below which a speed is considered zero (avoids floating-point noise) */
#define NEAR_ZERO 0.001f

/*------------------------------------------------------------------------------------
 * MoveUpdate (static)
 * 
 *   Per-tick movement update. Applied at fixed timestep (typically 60Hz).
 * 
 *   Two phases:
 *   1. Angular rotation: add angularSpeed × dt to the actor's Y rotation (yaw).
 *      This rotates the actor around its up axis.
 * 
 *   2. Linear movement: move along the actor's forward and right vectors.
 *      newPos = pos + forward × forwardSpeed × dt + right × strafeSpeed × dt
 * 
 *   Both phases use a NEAR_ZERO threshold to skip computation when speeds are
 *   effectively zero, avoiding unnecessary transform dirtying.
 * 
 *   Update order 10 ensures movement runs early, before mesh/collider updates.
 *------------------------------------------------------------------------------------*/
static void MOVE_COMPONENT_Update(Component* self, float deltaTime) 
{
	assert(self != NULL);
    if(self->type != COMPONENT_TYPE_MOVE) 
    {
        return;
	}

    MoveComponent* mc = (MoveComponent*)self;
    Actor* owner = self->owner;

    /* ── Phase 1: Angular rotation (yaw) ── */
    if (fabsf(mc->angularSpeed) > NEAR_ZERO) 
    {
        float angle = mc->angularSpeed * deltaTime;
        Vector3 rot = owner->root.rotation;
        rot.y += angle;
        ACTOR_SetRotation(owner, rot);
    }

    /* ── Phase 2: Linear movement (forward + strafe) ── */
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

/*------------------------------------------------------------------------------------
 * MOVE_COMPONENT_Create
 * 
 *   Factory function — allocates from the component pool, initializes with
 *   update order 10 (runs early), and sets all speeds to 0.
 *------------------------------------------------------------------------------------*/
MoveComponent* MOVE_COMPONENT_Create(Actor* owner)
{
	assert(owner != NULL);
    MoveComponent* mc = (MoveComponent*)MEMORY_AllocComponent(&owner->game->memory, sizeof(MoveComponent));
    if (!mc) return NULL;

    COMPONENT_Init(&mc->base, owner, COMPONENT_TYPE_MOVE, 10);
    mc->base.Update = MOVE_COMPONENT_Update;

    mc->angularSpeed = 0.0f;
    mc->forwardSpeed = 0.0f;
    mc->strafeSpeed = 0.0f;

    return mc;
}

/* Set all speed values. Positive forward = move along -Z, positive angular = turn left. */
void MOVE_COMPONENT_SetSpeeds(MoveComponent* mc, float forward, float angular, float strafe)
{
    assert(mc != NULL);
    mc->forwardSpeed = forward;
    mc->angularSpeed = angular;
    mc->strafeSpeed = strafe;
}