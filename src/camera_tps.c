#include "camera_tps.h"
#include "actor.h"
#include "game.h"
#include "memory.h"
#include <assert.h>
#include <math.h>

static Vector3 ComputeIdealPos(CameraTPS* ctps)
{
    assert(ctps != NULL);
    Actor* owner = CAMERA_COMPONENT_GetOwner(&ctps->cameraComponent);
    Vector3 pos = owner->root.position;
    Vector3 fwd = ACTOR_GetForward(owner);

    pos = Vector3Subtract(pos, Vector3Scale(fwd, ctps->horzDist));
    pos.y += ctps->vertDist;
    return pos;
}

static Vector3 ComputeTarget(CameraTPS* ctps)
{
    assert(ctps != NULL);
    Actor* owner = CAMERA_COMPONENT_GetOwner(&ctps->cameraComponent);
    Vector3 fwd = ACTOR_GetForward(owner);
    return Vector3Add(owner->root.position, Vector3Scale(fwd, ctps->targetDist));
}

static void CameraTPSUpdate(Component* self, float deltaTime)
{
    assert(self != NULL);
    if(self->type != COMPONENT_CAMERA_TPS) return;
    CameraTPS* ctps = (CameraTPS*)self;

    /* Critically damped spring: dampening = 2 * sqrt(k) */
    float dampening = 2.0f * sqrtf(ctps->springConstant);

    Vector3 idealPos = ComputeIdealPos(ctps);

    /* Spring acceleration: F = -k * displacement - dampening * velocity */
    Vector3 diff = Vector3Subtract(ctps->actualPos, idealPos);
    Vector3 accel = Vector3Subtract(
        Vector3Scale(diff, -ctps->springConstant),
        Vector3Scale(ctps->velocity, dampening)
    );

    /* Integrate */
    ctps->velocity = Vector3Add(ctps->velocity, Vector3Scale(accel, deltaTime));
    ctps->actualPos = Vector3Add(ctps->actualPos, Vector3Scale(ctps->velocity, deltaTime));

    /* Write result into the base Camera3D, then push to Renderer */
    ctps->cameraComponent.cam.position = ctps->actualPos;
    ctps->cameraComponent.cam.target = ComputeTarget(ctps);
    ctps->cameraComponent.cam.up = (Vector3){ 0.0f, 1.0f, 0.0f };

    CAMERA_COMPONENT_Apply(&ctps->cameraComponent);
}

CameraTPS* CAMERA_TPS_Create(Actor* owner)
{
    assert(owner != NULL);

    CameraTPS* ctps = (CameraTPS*)MEMORY_AllocComponent(
        &owner->game->memory, sizeof(CameraTPS));
    if (!ctps) return NULL;

    CAMERA_COMPONENT_Init(&ctps->cameraComponent, owner);
    ctps->cameraComponent.scene.base.type = COMPONENT_CAMERA_TPS;
    ctps->cameraComponent.scene.base.Update = CameraTPSUpdate;

    ctps->actualPos = (Vector3){ 0 };
    ctps->velocity = (Vector3){ 0 };

    /* Defaults tuned for unit-scale world (cubes are 1�1�1) */
    ctps->horzDist = 6.0f;
    ctps->vertDist = 4.0f;
    ctps->targetDist = 3.0f;
    ctps->springConstant = 64.0f;

    /* Start at ideal position � no spring lag on first frame */
    CAMERA_TPS_SnapToIdeal(ctps);

    return ctps;
}

void CAMERA_TPS_SnapToIdeal(CameraTPS* ctps)
{
    assert(ctps != NULL);

    ctps->actualPos = ComputeIdealPos(ctps);
    ctps->velocity = (Vector3){ 0 };

    ctps->cameraComponent.cam.position = ctps->actualPos;
    ctps->cameraComponent.cam.target = ComputeTarget(ctps);
    ctps->cameraComponent.cam.up = (Vector3){ 0.0f, 1.0f, 0.0f };

    CAMERA_COMPONENT_Apply(&ctps->cameraComponent);
}