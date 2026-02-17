#include "follow_camera.h"
#include "actor.h"
#include "game.h"
#include "memory.h"
#include <assert.h>
#include <math.h>

static Vector3 ComputeIdealPos(FollowCameraComponent* fc)
{
    Actor* owner = fc->base.base.owner;
    Vector3 pos = owner->position;
    Vector3 fwd = ACTOR_GetForward(owner);

    pos = Vector3Subtract(pos, Vector3Scale(fwd, fc->horzDist));
    pos.y += fc->vertDist;
    return pos;
}

static Vector3 ComputeTarget(FollowCameraComponent* fc)
{
    Actor* owner = fc->base.base.owner;
    Vector3 fwd = ACTOR_GetForward(owner);
    return Vector3Add(owner->position, Vector3Scale(fwd, fc->targetDist));
}

static void FollowCameraUpdate(Component* self, float deltaTime)
{
    assert(self != NULL);
    FollowCameraComponent* fc = (FollowCameraComponent*)self;

    /* Critically damped spring: dampening = 2 * sqrt(k) */
    float dampening = 2.0f * sqrtf(fc->springConstant);

    Vector3 idealPos = ComputeIdealPos(fc);

    /* Spring acceleration: F = -k * displacement - dampening * velocity */
    Vector3 diff = Vector3Subtract(fc->actualPos, idealPos);
    Vector3 accel = Vector3Subtract(
        Vector3Scale(diff, -fc->springConstant),
        Vector3Scale(fc->velocity, dampening)
    );

    /* Integrate */
    fc->velocity = Vector3Add(fc->velocity, Vector3Scale(accel, deltaTime));
    fc->actualPos = Vector3Add(fc->actualPos, Vector3Scale(fc->velocity, deltaTime));

    /* Write result into the base Camera3D, then push to Renderer */
    fc->base.cam.position = fc->actualPos;
    fc->base.cam.target = ComputeTarget(fc);
    fc->base.cam.up = (Vector3){ 0.0f, 1.0f, 0.0f };

    CAMERA_COMPONENT_Apply(&fc->base);
}

FollowCameraComponent* FOLLOW_CAMERA_Create(Actor* owner)
{
    assert(owner != NULL);

    FollowCameraComponent* fc = (FollowCameraComponent*)MEMORY_AllocComponent(
        &owner->game->memory, sizeof(FollowCameraComponent));
    if (!fc) return NULL;

    CAMERA_COMPONENT_Init(&fc->base, owner);
    fc->base.base.Update = FollowCameraUpdate;

    fc->actualPos = (Vector3){ 0 };
    fc->velocity = (Vector3){ 0 };

    /* Defaults tuned for unit-scale world (cubes are 1󪻑) */
    fc->horzDist = 6.0f;
    fc->vertDist = 4.0f;
    fc->targetDist = 3.0f;
    fc->springConstant = 64.0f;

    /* Start at ideal position � no spring lag on first frame */
    FOLLOW_CAMERA_SnapToIdeal(fc);

    return fc;
}

void FOLLOW_CAMERA_SnapToIdeal(FollowCameraComponent* fc)
{
    assert(fc != NULL);

    fc->actualPos = ComputeIdealPos(fc);
    fc->velocity = (Vector3){ 0 };

    fc->base.cam.position = fc->actualPos;
    fc->base.cam.target = ComputeTarget(fc);
    fc->base.cam.up = (Vector3){ 0.0f, 1.0f, 0.0f };

    CAMERA_COMPONENT_Apply(&fc->base);
}