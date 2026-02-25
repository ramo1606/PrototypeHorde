#include "physics_world.h"
#include "box_component.h"
#include "sphere_component.h"
#include "actor.h"
#include <assert.h>
#include <stdlib.h>
#include <float.h>

void PHYS_WORLD_Init(PhysWorld* world)
{
    /*
     * Zero all collider arrays and clear callbacks.  The colliders
     * register themselves individually when created, so nothing more
     * is needed here.
     */
    assert(world != NULL);
    world->boxCount = 0;
    world->sphereCount = 0;

    for (int i = 0; i < PHYS_WORLD_MAX_BOXES; i++)
        world->boxes[i] = NULL;
    for (int i = 0; i < PHYS_WORLD_MAX_SPHERES; i++)
        world->spheres[i] = NULL;

    world->onContact = NULL;
    world->onPairCollision = NULL;

    TraceLog(LOG_INFO, "PHYS_WORLD: Initialized (max %d boxes, %d spheres)",
        PHYS_WORLD_MAX_BOXES, PHYS_WORLD_MAX_SPHERES);
}

void PHYS_WORLD_Shutdown(PhysWorld* world)
{
    /*
     * Reset counters and callbacks.  Individual colliders are destroyed
     * (and unregistered) through the component system before Shutdown
     * is called, so the arrays should be empty by the time we reach here.
     */
    assert(world != NULL);
    world->boxCount = 0;
    world->sphereCount = 0;
    world->onContact = NULL;
    world->onPairCollision = NULL;

    TraceLog(LOG_INFO, "PHYS_WORLD: Shutdown");
}

void PHYS_WORLD_Update(PhysWorld* world, float deltaTime)
{
    /*
     * Run the broadphase collision pass.  Currently only the legacy
     * pairwise callback is supported.  Phase 5 will add a spatial grid
     * and the onContact detailed callback path.
     */
    assert(world != NULL);
    /* For now, we only do pairwise collision tests. Phase 5 will add a spatial grid. */
    if (world->onPairCollision)
    {
        PHYS_WORLD_TestPairwise(world, world->onPairCollision);
	}
}

void PHYS_WORLD_AddBox(PhysWorld* world, BoxComponent* box)
{
    /*
     * Append to the flat array.  Swap-remove is used in RemoveBox so
     * order is not preserved, which is fine for unordered broadphase.
     */
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
    /*
     * Swap-remove: replace the found entry with the last entry and
     * decrement count.  O(n) scan, O(1) remove; avoids shifting.
     */
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

bool PHYS_WORLD_RayCast(PhysWorld* world, Ray ray, float maxDist, uint32_t layerMask, CollisionInfo* outHit)
{
    /*
     * Brute-force ray test against all registered boxes and spheres.
     * Tracks the closest hit and fills outHit with its data.
     * layerMask filtering is reserved for Phase 5 (currently unused).
     */
    assert(world != NULL && outHit != NULL);

    bool collided = false;
    float closestDist = FLT_MAX;

    for (int i = 0; i < world->boxCount; i++)
    {
        BoundingBox wb = BOX_COMPONENT_GetWorldBox(world->boxes[i]);
        RayCollision rc = GetRayCollisionBox(ray, wb);

        if (rc.hit && rc.distance <= maxDist && rc.distance < closestDist)
        {
            closestDist = rc.distance;
            outHit->point = rc.point;
            outHit->normal = rc.normal;
            outHit->collider = (Component*)world->boxes[i];
            outHit->actor = world->boxes[i]->base.owner;
            collided = true;
        }
    }

    for (int i = 0; i < world->sphereCount; i++)
    {
        Vector3 center = SPHERE_COMPONENT_GetWorldCenter(world->spheres[i]);
        float   radius = SPHERE_COMPONENT_GetWorldRadius(world->spheres[i]);
        RayCollision rc = GetRayCollisionSphere(ray, center, radius);

        if (rc.hit && rc.distance <= maxDist && rc.distance < closestDist)
        {
            closestDist = rc.distance;
            outHit->point = rc.point;
            outHit->normal = rc.normal;
            outHit->collider = (Component*)world->spheres[i];
            outHit->actor = world->spheres[i]->base.owner;
            collided = true;
        }
    }

    if (collided)
    {
        outHit->hit = true;
        outHit->distance = closestDist;
    }

    return collided;
}

bool PHYS_WORLD_RayCastIgnore(PhysWorld* world, Ray ray, float maxDist, uint32_t layerMask, Actor* ignore, CollisionInfo* outHit)
{
    /*
     * Same as RayCast but skips any collider whose owner matches ignore.
     * Used by CameraTPS to avoid the player actor blocking the wall-check
     * ray.
     */
    assert(world != NULL && outHit != NULL);

    bool collided = false;
    float closestDist = FLT_MAX;

    for (int i = 0; i < world->boxCount; i++)
    {
        if (world->boxes[i]->base.owner == ignore) continue;
        BoundingBox wb = BOX_COMPONENT_GetWorldBox(world->boxes[i]);
        RayCollision rc = GetRayCollisionBox(ray, wb);

        if (rc.hit && rc.distance <= maxDist && rc.distance < closestDist)
        {
            closestDist = rc.distance;
            outHit->point = rc.point;
            outHit->normal = rc.normal;
            outHit->collider = (Component*)world->boxes[i];
            outHit->actor = world->boxes[i]->base.owner;
            collided = true;
        }
    }

    for (int i = 0; i < world->sphereCount; i++)
    {
        if (world->spheres[i]->base.owner == ignore) continue;
        Vector3 center = SPHERE_COMPONENT_GetWorldCenter(world->spheres[i]);
        float   radius = SPHERE_COMPONENT_GetWorldRadius(world->spheres[i]);
        RayCollision rc = GetRayCollisionSphere(ray, center, radius);

        if (rc.hit && rc.distance <= maxDist && rc.distance < closestDist)
        {
            closestDist = rc.distance;
            outHit->point = rc.point;
            outHit->normal = rc.normal;
            outHit->collider = (Component*)world->spheres[i];
            outHit->actor = world->spheres[i]->base.owner;
            collided = true;
        }
    }

    if (collided)
    {
        outHit->hit = true;
        outHit->distance = closestDist;
    }

    return collided;
}

int PHYS_WORLD_OverlapSphere(PhysWorld* world, Vector3 center, float radius, uint32_t layerMask, Actor** outActors, int maxResults)
{
    return 0;
}

int PHYS_WORLD_OverlapBox(PhysWorld* world, BoundingBox box, uint32_t layerMask, Actor** outActors, int maxResults)
{
    return 0;
}

bool PHYS_WORLD_SphereCast(PhysWorld* world, Vector3 origin, float radius, Vector3 direction, float maxDist, uint32_t layerMask, CollisionInfo* outHit)
{
    return false;
}

void PHYS_WORLD_TestPairwise(PhysWorld* world, CollisionPairFn fn)
{
    /*
     * Brute-force O(n²) broadphase.
     *
     * Tests every unique pair within each shape type (box-box,
     * sphere-sphere) plus all cross-type pairs (box-sphere).  Fires fn
     * for each overlapping pair.  Simple and correct for small n.
     * For large scenes use TestSweepAndPrune instead.
     */
    assert(world != NULL && fn != NULL);

    for (int i = 0; i < world->boxCount; i++)
    {
        BoundingBox a = BOX_COMPONENT_GetWorldBox(world->boxes[i]);
        for (int j = i + 1; j < world->boxCount; j++)
        {
            BoundingBox b = BOX_COMPONENT_GetWorldBox(world->boxes[j]);
            if (CheckCollisionBoxes(a, b))
            {
                fn(world->boxes[i]->base.owner,
				   world->boxes[j]->base.owner,
                    (Vector3) {0, 0, 0}, 0.0f);
            }
        }
    }

    for (int i = 0; i < world->sphereCount; i++)
    {
        Vector3 ca = SPHERE_COMPONENT_GetWorldCenter(world->spheres[i]);
        float   ra = SPHERE_COMPONENT_GetWorldRadius(world->spheres[i]);
        for (int j = i + 1; j < world->sphereCount; j++)
        {
            Vector3 cb = SPHERE_COMPONENT_GetWorldCenter(world->spheres[j]);
            float   rb = SPHERE_COMPONENT_GetWorldRadius(world->spheres[j]);
            if (CheckCollisionSpheres(ca, ra, cb, rb))
            {
                fn(world->spheres[i]->base.owner,
                    world->spheres[j]->base.owner,
                    (Vector3) {0, 0, 0}, 0.0f);
            }
        }
    }

    for (int i = 0; i < world->boxCount; i++)
    {
        BoundingBox wb = BOX_COMPONENT_GetWorldBox(world->boxes[i]);
        for (int j = 0; j < world->sphereCount; j++)
        {
            Vector3 center = SPHERE_COMPONENT_GetWorldCenter(world->spheres[j]);
            float   radius = SPHERE_COMPONENT_GetWorldRadius(world->spheres[j]);
            if (CheckCollisionBoxSphere(wb, center, radius))
            {
                fn(world->boxes[i]->base.owner,
                    world->spheres[j]->base.owner, 
                    (Vector3) { 0, 0, 0 }, 0.0f);
            }
        }
    }
}

static int CompareMinX(const void* a, const void* b)
{
    /*
     * Comparator for qsort -- sorts BoxComponents ascending by their
     * world-space AABB min.x, enabling the sweep-and-prune early exit.
     */
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
    /*
     * Sweep-and-prune broadphase on the X axis for box-box pairs.
     *
     * Algorithm:
     *   1. Sort boxes by min.x using qsort (O(n log n)).
     *   2. For each box i, sweep forward through j > i.  As soon as
     *      box j's min.x exceeds box i's max.x, the interval on X is
     *      exhausted -- no further box can overlap box i on X, so break.
     *   3. For pairs that pass the X interval test, perform a full AABB
     *      overlap test and fire fn on hit.
     *
     * Box-sphere and sphere-sphere pairs still use brute force because
     * the sort only applies to boxes.
     *
     * Reference: "Real-Time Collision Detection" (Ericson), Ch. 7.
     */
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
                fn(world->boxes[i]->base.owner,
                    world->boxes[j]->base.owner,
                    (Vector3) {0, 0, 0}, 0.0f);
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
            float   radius = SPHERE_COMPONENT_GetWorldRadius(world->spheres[j]);
            if (CheckCollisionBoxSphere(wb, center, radius))
            {
                fn(world->boxes[i]->base.owner,
                    world->spheres[j]->base.owner,
                    (Vector3) {0, 0, 0}, 0.0f);
            }
        }
    }

    /* Sphere vs sphere brute force */
    for (int i = 0; i < world->sphereCount; i++)
    {
        Vector3 ca = SPHERE_COMPONENT_GetWorldCenter(world->spheres[i]);
        float   ra = SPHERE_COMPONENT_GetWorldRadius(world->spheres[i]);
        for (int j = i + 1; j < world->sphereCount; j++)
        {
            Vector3 cb = SPHERE_COMPONENT_GetWorldCenter(world->spheres[j]);
            float   rb = SPHERE_COMPONENT_GetWorldRadius(world->spheres[j]);
            if (CheckCollisionSpheres(ca, ra, cb, rb))
            {
                fn(world->spheres[i]->base.owner,
                    world->spheres[j]->base.owner,
                    (Vector3) {0, 0, 0}, 0.0f);
            }
        }
    }
}
