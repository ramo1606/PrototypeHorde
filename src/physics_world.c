#include "physics_world.h"
#include "box_component.h"
#include "sphere_component.h"
#include "actor.h"
#include <assert.h>
#include <stdlib.h>
#include <float.h>

/* ── Lifecycle ────────────────────────────────────────────────── */

void PHYS_WORLD_Init(PhysWorld* world)
{
    assert(world != NULL);
    world->boxCount = 0;
    world->sphereCount = 0;

    for (int i = 0; i < PHYS_WORLD_MAX_BOXES; i++)
        world->boxes[i] = NULL;
    for (int i = 0; i < PHYS_WORLD_MAX_SPHERES; i++)
        world->spheres[i] = NULL;

    TraceLog(LOG_INFO, "PHYS_WORLD: Initialized (max %d boxes, %d spheres)",
        PHYS_WORLD_MAX_BOXES, PHYS_WORLD_MAX_SPHERES);
}

/* ── Registration ─────────────────────────────────────────────── */

void PHYS_WORLD_AddBox(PhysWorld* world, BoxComponent* box)
{
    assert(world != NULL && box != NULL);
    if (world->boxCount >= PHYS_WORLD_MAX_BOXES)
    {
        TraceLog(LOG_WARNING, "PHYS_WORLD: Box list full (%d)", PHYS_WORLD_MAX_BOXES);
        return;
    }
    world->boxes[world->boxCount++] = box;
}

void PHYS_WORLD_RemoveBox(PhysWorld* world, BoxComponent* box)
{
    assert(world != NULL && box != NULL);
    for (int i = 0; i < world->boxCount; i++)
    {
        if (world->boxes[i] == box)
        {
            world->boxes[i] = world->boxes[world->boxCount - 1];
            world->boxes[world->boxCount - 1] = NULL;
            world->boxCount--;
            return;
        }
    }
}

void PHYS_WORLD_AddSphere(PhysWorld* world, SphereComponent* sphere)
{
    assert(world != NULL && sphere != NULL);
    if (world->sphereCount >= PHYS_WORLD_MAX_SPHERES)
    {
        TraceLog(LOG_WARNING, "PHYS_WORLD: Sphere list full (%d)", PHYS_WORLD_MAX_SPHERES);
        return;
    }
    world->spheres[world->sphereCount++] = sphere;
}

void PHYS_WORLD_RemoveSphere(PhysWorld* world, SphereComponent* sphere)
{
    assert(world != NULL && sphere != NULL);
    for (int i = 0; i < world->sphereCount; i++)
    {
        if (world->spheres[i] == sphere)
        {
            world->spheres[i] = world->spheres[world->sphereCount - 1];
            world->spheres[world->sphereCount - 1] = NULL;
            world->sphereCount--;
            return;
        }
    }
}

/* ── RayCast ──────────────────────────────────────────────────── */

bool PHYS_WORLD_RayCast(PhysWorld* world, Ray ray, float maxDistance,
    CollisionInfo* outColl)
{
    assert(world != NULL && outColl != NULL);

    bool collided = false;
    float closestDist = FLT_MAX;

    /* Test boxes */
    for (int i = 0; i < world->boxCount; i++)
    {
        BoundingBox wb = BOX_COMPONENT_GetWorldBox(world->boxes[i]);
        RayCollision rc = GetRayCollisionBox(ray, wb);

        if (rc.hit && rc.distance <= maxDistance && rc.distance < closestDist)
        {
            closestDist = rc.distance;
            outColl->collision.point = rc.point;
            outColl->collision.normal = rc.normal;
            outColl->collider = (Component*)world->boxes[i];
            outColl->actor = world->boxes[i]->component.owner;
            collided = true;
        }
    }

    /* Test spheres */
    for (int i = 0; i < world->sphereCount; i++)
    {
        Vector3 center = SPHERE_COMPONENT_GetWorldCenter(world->spheres[i]);
        float   radius = SPHERE_COMPONENT_GetRadius(world->spheres[i]);
        RayCollision rc = GetRayCollisionSphere(ray, center, radius);

        if (rc.hit && rc.distance <= maxDistance && rc.distance < closestDist)
        {
            closestDist = rc.distance;
            outColl->collision.point = rc.point;
            outColl->collision.normal = rc.normal;
            outColl->collider = (Component*)world->spheres[i];
            outColl->actor = world->spheres[i]->component.owner;
            collided = true;
        }
    }

    return collided;
}

/* ── Pairwise ─────────────────────────────────────────────────── */

void PHYS_WORLD_TestPairwise(PhysWorld* world, CollisionPairFn fn)
{
    assert(world != NULL && fn != NULL);

    /* Box vs Box */
    for (int i = 0; i < world->boxCount; i++)
    {
        BoundingBox a = BOX_COMPONENT_GetWorldBox(world->boxes[i]);
        for (int j = i + 1; j < world->boxCount; j++)
        {
            BoundingBox b = BOX_COMPONENT_GetWorldBox(world->boxes[j]);
            if (CheckCollisionBoxes(a, b))
            {
                fn(world->boxes[i]->component.owner,
                    world->boxes[j]->component.owner);
            }
        }
    }

    /* Sphere vs Sphere */
    for (int i = 0; i < world->sphereCount; i++)
    {
        Vector3 ca = SPHERE_COMPONENT_GetWorldCenter(world->spheres[i]);
        float   ra = SPHERE_COMPONENT_GetRadius(world->spheres[i]);
        for (int j = i + 1; j < world->sphereCount; j++)
        {
            Vector3 cb = SPHERE_COMPONENT_GetWorldCenter(world->spheres[j]);
            float   rb = SPHERE_COMPONENT_GetRadius(world->spheres[j]);
            if (CheckCollisionSpheres(ca, ra, cb, rb))
            {
                fn(world->spheres[i]->component.owner,
                    world->spheres[j]->component.owner);
            }
        }
    }

    /* Box vs Sphere */
    for (int i = 0; i < world->boxCount; i++)
    {
        BoundingBox wb = BOX_COMPONENT_GetWorldBox(world->boxes[i]);
        for (int j = 0; j < world->sphereCount; j++)
        {
            Vector3 center = SPHERE_COMPONENT_GetWorldCenter(world->spheres[j]);
            float   radius = SPHERE_COMPONENT_GetRadius(world->spheres[j]);
            if (CheckCollisionBoxSphere(wb, center, radius))
            {
                fn(world->boxes[i]->component.owner,
                    world->spheres[j]->component.owner);
            }
        }
    }
}

/* ── Sweep-and-prune (box-only optimization) ──────────────────── */

static int CompareMinX(const void* a, const void* b)
{
    BoxComponent* ba = *(BoxComponent**)a;
    BoxComponent* bb = *(BoxComponent**)b;
    float ax = BOX_COMPONENT_GetWorldBox(ba).min.x;
    float bx = BOX_COMPONENT_GetWorldBox(bb).min.x;
    if (ax < bx) return -1;
    if (ax > bx) return  1;
    return 0;
}

void PHYS_WORLD_TestSweepAndPrune(PhysWorld* world, CollisionPairFn fn)
{
    assert(world != NULL && fn != NULL);

    if (world->boxCount < 2) return;

    qsort(world->boxes, world->boxCount, sizeof(BoxComponent*), CompareMinX);

    for (int i = 0; i < world->boxCount; i++)
    {
        BoundingBox a = BOX_COMPONENT_GetWorldBox(world->boxes[i]);
        float maxX = a.max.x;

        for (int j = i + 1; j < world->boxCount; j++)
        {
            BoundingBox b = BOX_COMPONENT_GetWorldBox(world->boxes[j]);
            if (b.min.x > maxX) break;

            if (CheckCollisionBoxes(a, b))
            {
                fn(world->boxes[i]->component.owner,
                    world->boxes[j]->component.owner);
            }
        }
    }

    /* Cross-type: box vs sphere still uses brute force */
    for (int i = 0; i < world->boxCount; i++)
    {
        BoundingBox wb = BOX_COMPONENT_GetWorldBox(world->boxes[i]);
        for (int j = 0; j < world->sphereCount; j++)
        {
            Vector3 center = SPHERE_COMPONENT_GetWorldCenter(world->spheres[j]);
            float   radius = SPHERE_COMPONENT_GetRadius(world->spheres[j]);
            if (CheckCollisionBoxSphere(wb, center, radius))
            {
                fn(world->boxes[i]->component.owner,
                    world->spheres[j]->component.owner);
            }
        }
    }

    /* Sphere vs sphere brute force */
    for (int i = 0; i < world->sphereCount; i++)
    {
        Vector3 ca = SPHERE_COMPONENT_GetWorldCenter(world->spheres[i]);
        float   ra = SPHERE_COMPONENT_GetRadius(world->spheres[i]);
        for (int j = i + 1; j < world->sphereCount; j++)
        {
            Vector3 cb = SPHERE_COMPONENT_GetWorldCenter(world->spheres[j]);
            float   rb = SPHERE_COMPONENT_GetRadius(world->spheres[j]);
            if (CheckCollisionSpheres(ca, ra, cb, rb))
            {
                fn(world->spheres[i]->component.owner,
                    world->spheres[j]->component.owner);
            }
        }
    }
}