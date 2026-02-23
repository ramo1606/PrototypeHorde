#include "camera_tps.h"
#include "actor.h"
#include "game.h"
#include "memory.h"
#include <assert.h>
#include <math.h>

/* Small offset to keep camera in front of the wall surface */
#define CAMERA_WALL_OFFSET 0.25f

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

/*
 * Raycast from the look-at target toward the ideal camera position.
 * If geometry is hit, pull the camera in front of the hit point.
 * This prevents the camera from ending up behind walls/floors.
 */
static Vector3 ClampCameraToWorld(CameraTPS* ctps, Vector3 cameraPos)
{
    Actor* owner = CAMERA_COMPONENT_GetOwner(&ctps->cameraComponent);
    PhysWorld* world = &owner->game->physWorld;

    /* Ray from target toward camera */
    Vector3 target = ComputeTarget(ctps);
    Vector3 toCamera = Vector3Subtract(cameraPos, target);
    float dist = Vector3Length(toCamera);
    if (dist < 0.001f) return cameraPos;

    Ray ray = { target, Vector3Scale(toCamera, 1.0f / dist) };
    CollisionInfo hit;

    if (PHYS_WORLD_RayCast(world, ray, dist, &hit))
    {
        /* Place camera at hit point minus a small offset toward target */
        float clampedDist = hit.collision.distance - CAMERA_WALL_OFFSET;
        if (clampedDist < CAMERA_WALL_OFFSET) clampedDist = CAMERA_WALL_OFFSET;
        return Vector3Add(target, Vector3Scale(ray.direction, clampedDist));
    }

    return cameraPos;
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

    /* Prevent camera from clipping through world geometry */
    ctps->actualPos = ClampCameraToWorld(ctps, ctps->actualPos);

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