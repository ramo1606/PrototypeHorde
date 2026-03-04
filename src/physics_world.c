/*******************************************************************************************
*
*   physics_world.c — Physics World Implementation
*
********************************************************************************************/
#include "physics_world.h"
#include "box_component.h"
#include "sphere_component.h"
#include "actor.h"
#include <assert.h>
#include <stdlib.h>
#include <float.h>

/* ═══════════════════════════════════════════════════════════════════════════
 *  Lifecycle
 * ═══════════════════════════════════════════════════════════════════════════ */

/*------------------------------------------------------------------------------------
 * PHYS_WORLD_Init
 * 
 *   Initializes all collider lists to empty and clears callback pointers.
 *   Called once during GAME_Init.
 *------------------------------------------------------------------------------------*/
void PHYS_WORLD_Init(PhysWorld* world)
{
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

/*------------------------------------------------------------------------------------
 * PHYS_WORLD_Shutdown
 * 
 *   Clears all references. Does NOT destroy collider components — that's handled
 *   by GAME_RemoveAllActors which destroys actors (and their components).
 *------------------------------------------------------------------------------------*/
void PHYS_WORLD_Shutdown(PhysWorld* world)
{
    assert(world != NULL);
    world->boxCount = 0;
    world->sphereCount = 0;
    world->onContact = NULL;
    world->onPairCollision = NULL;

    TraceLog(LOG_INFO, "PHYS_WORLD: Shutdown");
}

/*------------------------------------------------------------------------------------
 * PHYS_WORLD_Update
 * 
 *   Per-tick physics update. Currently only runs pairwise collision if a callback
 *   is registered. Phase 5 will add spatial grid acceleration.
 *------------------------------------------------------------------------------------*/
void PHYS_WORLD_Update(PhysWorld* world, float deltaTime)
{
    assert(world != NULL);
    /* For now, we only do pairwise collision tests. Phase 5 will add a spatial grid. */
    if (world->onPairCollision)
    {
        PHYS_WORLD_TestPairwise(world, world->onPairCollision);
	}
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Collider Registration
 * 
 *  Add/Remove use a simple append-to-end / swap-with-last pattern.
 *  Swap-with-last is O(1) but does NOT preserve insertion order.
 *  This is fine since collider ordering doesn't affect correctness.
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Register a box collider. Called by BOX_COMPONENT_Create. */
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

/* Unregister a box collider. Uses swap-with-last for O(1) removal. */
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

/* Register a sphere collider. Called by SPHERE_COMPONENT_Create. */
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

/* Unregister a sphere collider. Uses swap-with-last for O(1) removal. */
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

/* ═══════════════════════════════════════════════════════════════════════════
 *  Ray Casting
 * 
 *  Casts a ray against all registered colliders and returns the closest hit.
 *  Uses Raylib's GetRayCollisionBox and GetRayCollisionSphere for the actual
 *  ray-shape intersection math.
 * 
 *  Current implementation: brute force O(n) — tests every collider.
 *  Phase 5 will use a spatial grid for early rejection.
 * ═══════════════════════════════════════════════════════════════════════════ */

/*------------------------------------------------------------------------------------
 * PHYS_WORLD_RayCast
 * 
 *   Casts a ray against all boxes and spheres, returns the closest hit.
 * 
 *   Algorithm:
 *   1. Iterate all box colliders: get world AABB, test ray vs box.
 *   2. Iterate all sphere colliders: get world center/radius, test ray vs sphere.
 *   3. Track the closest hit across all tests.
 *   4. Return true if any hit was found within maxDist.
 * 
 *   The layerMask parameter is available for Phase 5 filtering but is
 *   currently not checked (all colliders are tested regardless).
 *------------------------------------------------------------------------------------*/
bool PHYS_WORLD_RayCast(PhysWorld* world, Ray ray, float maxDist, uint32_t layerMask, CollisionInfo* outHit)
{
    assert(world != NULL && outHit != NULL);

    bool collided = false;
    float closestDist = FLT_MAX;

    /* ── Test against all box colliders ── */
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

    /* ── Test against all sphere colliders ── */
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

/*------------------------------------------------------------------------------------
 * PHYS_WORLD_RayCastIgnore
 * 
 *   Same as RayCast but skips colliders owned by the 'ignore' actor.
 *   Used when an actor casts a ray from its own position (e.g., the TPS camera
 *   checking for wall occlusion — it needs to ignore the player's own collider).
 * 
 *   Stub: returns false (not yet implemented).
 *------------------------------------------------------------------------------------*/
bool PHYS_WORLD_RayCastIgnore(PhysWorld* world, Ray ray, float maxDist, uint32_t layerMask, Actor* ignore, CollisionInfo* outHit)
{
    return false;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Overlap Queries — Stubs (Phase 5)
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Stub: Find all actors within a sphere region. */
int PHYS_WORLD_OverlapSphere(PhysWorld* world, Vector3 center, float radius, uint32_t layerMask, Actor** outActors, int maxResults)
{
    return 0;
}

/* Stub: Find all actors within an AABB region. */
int PHYS_WORLD_OverlapBox(PhysWorld* world, BoundingBox box, uint32_t layerMask, Actor** outActors, int maxResults)
{
    return 0;
}

/* Stub: Cast a sphere along a direction and find the first hit. */
bool PHYS_WORLD_SphereCast(PhysWorld* world, Vector3 origin, float radius, Vector3 direction, float maxDist, uint32_t layerMask, CollisionInfo* outHit)
{
    return false;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Pairwise Collision Testing
 * 
 *  Two algorithms are available for all-vs-all collision testing:
 * 
 *  1. TestPairwise (Brute Force):
 *     Tests every unique pair: box-box, sphere-sphere, box-sphere.
 *     O(n²) time complexity. Simple and correct, good for small scenes.
 * 
 *  2. TestSweepAndPrune:
 *     Sorts boxes by min.x, then only tests pairs whose X ranges overlap.
 *     O(n log n) for the sort, then early exit on X axis mismatch.
 *     Cross-type (box-sphere, sphere-sphere) still uses brute force.
 * 
 *  Both call the provided callback function for each overlapping pair.
 *  The callback receives the two actors, a normal, and penetration depth
 *  (currently zero — Phase 5 will compute actual penetration).
 * ═══════════════════════════════════════════════════════════════════════════ */

/*------------------------------------------------------------------------------------
 * PHYS_WORLD_TestPairwise
 * 
 *   Brute force O(n²) pairwise collision test.
 *   Tests all three pair combinations:
 *     1. Box vs Box     (n_boxes choose 2)
 *     2. Sphere vs Sphere (n_spheres choose 2)
 *     3. Box vs Sphere    (n_boxes × n_spheres)
 * 
 *   Uses Raylib's CheckCollisionBoxes and CheckCollisionBoxSphere for the
 *   actual intersection math.
 * 
 *   Note: Uses GetWorldRadius for sphere tests to handle scaled actors correctly.
 *------------------------------------------------------------------------------------*/
void PHYS_WORLD_TestPairwise(PhysWorld* world, CollisionPairFn fn)
{
    assert(world != NULL && fn != NULL);

    /* ── Box vs Box ── */
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

    /* ── Sphere vs Sphere ── */
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

    /* ── Box vs Sphere ── */
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

/* ── Sweep-and-Prune comparator ── */
/* qsort comparator: sorts BoxComponent pointers by their world AABB's min.x value. */
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

/*------------------------------------------------------------------------------------
 * PHYS_WORLD_TestSweepAndPrune
 * 
 *   Sweep-and-Prune (SAP) broad phase collision algorithm for box-box pairs.
 * 
 *   Algorithm:
 *   1. Sort all boxes by their minimum X coordinate.
 *   2. For each box A, only test box B if B.min.x <= A.max.x.
 *      Since boxes are sorted, once B.min.x > A.max.x, all subsequent boxes
 *      are also beyond A's X range — we can BREAK the inner loop early.
 *   3. For overlapping X ranges, do the full 3D AABB overlap test.
 * 
 *   Complexity:
 *       O(n log n) for the sort + O(n × k) for the pairwise tests,
 *       where k is the average number of X-overlapping neighbors.
 *       In sparse scenes, k << n, making this much faster than O(n²).
 * 
 *   Limitation:
 *       SAP only accelerates box-box tests. Cross-type (box-sphere) and
 *       sphere-sphere still use brute force. A spatial grid (Phase 5) would
 *       accelerate all collision types uniformly.
 * 
 *   Reference: "Real-Time Collision Detection" by Christer Ericson, Chapter 7
 *------------------------------------------------------------------------------------*/
void PHYS_WORLD_TestSweepAndPrune(PhysWorld* world, CollisionPairFn fn)
{
    assert(world != NULL && fn != NULL);

    if (world->boxCount < 2) return;

    /* ── Step 1: Sort boxes by min.x ── */
    qsort(world->boxes, world->boxCount, sizeof(BoxComponent*), CompareMinX);

    /* ── Step 2: Sweep along X axis with early exit ── */
    for (int i = 0; i < world->boxCount; i++)
    {
        BoundingBox a = BOX_COMPONENT_GetWorldBox(world->boxes[i]);
        float maxX = a.max.x;

        for (int j = i + 1; j < world->boxCount; j++)
        {
            BoundingBox b = BOX_COMPONENT_GetWorldBox(world->boxes[j]);

            /* Early exit: all remaining boxes have min.x > maxX (sorted) */
            if (b.min.x > maxX) break;

            /* Full 3D overlap test for the narrow phase */
            if (CheckCollisionBoxes(a, b))
            {
                fn(world->boxes[i]->base.owner,
                    world->boxes[j]->base.owner,
                    (Vector3) {0, 0, 0}, 0.0f);
            }
        }
    }

    /* ── Cross-type: box vs sphere (brute force) ── */
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