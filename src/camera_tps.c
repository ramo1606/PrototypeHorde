#include "camera_tps.h"
#include "actor.h"
#include "game.h"
#include "memory.h"
#include <assert.h>
#include <math.h>

#define CAMERA_WALL_OFFSET 0.25f

static Vector3 ComputeIdealPos(CameraTPS* ctps)
{
    assert(ctps != NULL);
    Actor* owner = CAMERA_COMPONENT_GetOwner(&ctps->base);
    Vector3 pos = owner->root.position;
    Vector3 fwd = ACTOR_GetForward(owner);

    pos = Vector3Subtract(pos, Vector3Scale(fwd, ctps->horzDist));
    pos.y += ctps->vertDist;
    return pos;
}

static Vector3 ComputeTarget(CameraTPS* ctps)
{
    assert(ctps != NULL);
    Actor* owner = CAMERA_COMPONENT_GetOwner(&ctps->base);
    Vector3 fwd = ACTOR_GetForward(owner);
    return Vector3Add(owner->root.position, Vector3Scale(fwd, ctps->targetDist));
}

static Vector3 ClampCameraToWorld(CameraTPS* ctps, Vector3 cameraPos)
{
    Actor* owner = CAMERA_COMPONENT_GetOwner(&ctps->base);
    PhysWorld* world = &owner->game->physWorld;

    Vector3 target = ComputeTarget(ctps);
    Vector3 toCamera = Vector3Subtract(cameraPos, target);
    float dist = Vector3Length(toCamera);
    if (dist < 0.001f) return cameraPos;

    Ray ray = { target, Vector3Scale(toCamera, 1.0f / dist) };
    CollisionInfo hit;

    if (PHYS_WORLD_RayCast(world, ray, dist, 0xFFFFFFFF, &hit))
    {
        float clampedDist = hit.distance - CAMERA_WALL_OFFSET;
        if (clampedDist < CAMERA_WALL_OFFSET) clampedDist = CAMERA_WALL_OFFSET;
        return Vector3Add(target, Vector3Scale(ray.direction, clampedDist));
    }

    return cameraPos;
}

static void CameraTPSUpdate(Component* self, float deltaTime)
{
    assert(self != NULL);
    if(self->type != COMPONENT_TYPE_CAMERA_TPS) return;
    CameraTPS* ctps = (CameraTPS*)self;

    float dampening = 2.0f * sqrtf(ctps->springConstant);

    Vector3 idealPos = ComputeIdealPos(ctps);

    Vector3 diff = Vector3Subtract(ctps->actualPos, idealPos);
    Vector3 accel = Vector3Subtract(
        Vector3Scale(diff, -ctps->springConstant),
        Vector3Scale(ctps->velocity, dampening)
    );

    ctps->velocity = Vector3Add(ctps->velocity, Vector3Scale(accel, deltaTime));
    ctps->actualPos = Vector3Add(ctps->actualPos, Vector3Scale(ctps->velocity, deltaTime));

    ctps->actualPos = ClampCameraToWorld(ctps, ctps->actualPos);

    ctps->base.camera.position = ctps->actualPos;
    ctps->base.camera.target = ComputeTarget(ctps);
    ctps->base.camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };

    CAMERA_COMPONENT_Apply(&ctps->base);
}

CameraTPS* CAMERA_TPS_Create(Actor* owner)
{
    assert(owner != NULL);

    CameraTPS* ctps = (CameraTPS*)MEMORY_AllocComponent(
        &owner->game->memory, sizeof(CameraTPS));
    if (!ctps) return NULL;

    CAMERA_COMPONENT_Init(&ctps->base, owner);
    ctps->base.scene.base.type = COMPONENT_TYPE_CAMERA_TPS;
    ctps->base.scene.base.Update = CameraTPSUpdate;

    ctps->actualPos = (Vector3){ 0 };
    ctps->velocity = (Vector3){ 0 };

    ctps->horzDist = 6.0f;
    ctps->vertDist = 4.0f;
    ctps->targetDist = 3.0f;
    ctps->springConstant = 64.0f;

    CAMERA_TPS_SnapToIdeal(ctps);

    return ctps;
}

void CAMERA_TPS_SnapToIdeal(CameraTPS* ctps)
{
    assert(ctps != NULL);

    ctps->actualPos = ComputeIdealPos(ctps);
    ctps->velocity = (Vector3){ 0 };

    ctps->base.camera.position = ctps->actualPos;
    ctps->base.camera.target = ComputeTarget(ctps);
    ctps->base.camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };

    CAMERA_COMPONENT_Apply(&ctps->base);
}

void CAMERA_TPS_SetDistances(CameraTPS* ctps, float horz, float vert, float target)
{
    assert(ctps != NULL);
    ctps->horzDist = horz;
    ctps->vertDist = vert;
	ctps->targetDist = target;
}

void CAMERA_TPS_SetSpring(CameraTPS* ctps, float springConstant)
{
    assert(ctps != NULL);
	ctps->springConstant = springConstant;
}
