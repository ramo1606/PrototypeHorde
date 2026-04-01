#include "physics.h"
#include "raylib.h"

#include <assert.h>
#include <string.h>
#include <math.h>

/*******************************************************************************************
*
*   physics.c — Physics World: Colliders, Detection, and Queries
*
*   Task 2.1: Structs defined in physics.h ✓
*   Task 2.2: Collider pool, registration, position updates.
*   Task 2.3: 6 collision test functions (pure geometry).
*   Task 2.4: PHYS_WORLD_Update + MoveAndCollide.
*   Task 2.5: Raycast and overlap queries.
*   Task 2.6: Debug wireframe drawing + panel F4.
*
********************************************************************************************/

/* ═══════════════════════════════════════════════════════════════════════════
 *  Lifecycle
 * ═══════════════════════════════════════════════════════════════════════════ */

void PHYS_WORLD_Init(PhysWorld* world)
{
    assert(world);
    memset(world, 0, sizeof(*world));
    TraceLog(LOG_INFO, "PHYS_WORLD: Initialized (max %d colliders)", MAX_COLLIDERS);
}

void PHYS_WORLD_Shutdown(PhysWorld* world)
{
    assert(world);
    memset(world->colliders, 0, sizeof(world->colliders));
    world->colliderCount = 0;
    TraceLog(LOG_INFO, "PHYS_WORLD: Shutdown");
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Registration
 *
 *  Same pattern as the renderer: find first inactive slot, fill it,
 *  return the index as a handle.
 * ═══════════════════════════════════════════════════════════════════════════ */

static ColliderHandle FindFreeSlot(PhysWorld* world)
{
    for (int i = 0; i < MAX_COLLIDERS; i++)
    {
        if (!world->colliders[i].active) return i;
    }
    TraceLog(LOG_WARNING, "PHYS_WORLD: Pool full (max %d)", MAX_COLLIDERS);
    return COLLIDER_HANDLE_INVALID;
}

ColliderHandle PHYS_WORLD_AddBox(PhysWorld* world, Vector3 position,
    Vector3 halfExtents, int layer, int mask, int ownerID,
    bool dynamic, bool trigger)
{
    assert(world);
    ColliderHandle h = FindFreeSlot(world);
    if (h == COLLIDER_HANDLE_INVALID) return h;

    Collider* c = &world->colliders[h];
    memset(c, 0, sizeof(*c));

    c->type = COLLIDER_AABB;
    c->shape.box.halfExtents = halfExtents;
    c->position = position;
    c->layer    = layer;
    c->mask     = mask;
    c->ownerID  = ownerID;
    c->active   = true;
    c->dynamic  = dynamic;
    c->trigger  = trigger;

    world->colliderCount++;
    TraceLog(LOG_DEBUG, "PHYS_WORLD: Added box handle %d (total: %d)", h, world->colliderCount);
    return h;
}

ColliderHandle PHYS_WORLD_AddSphere(PhysWorld* world, Vector3 position,
    float radius, int layer, int mask, int ownerID,
    bool dynamic, bool trigger)
{
    assert(world);
    ColliderHandle h = FindFreeSlot(world);
    if (h == COLLIDER_HANDLE_INVALID) return h;

    Collider* c = &world->colliders[h];
    memset(c, 0, sizeof(*c));

    c->type = COLLIDER_SPHERE;
    c->shape.sphere.radius = radius;
    c->position = position;
    c->layer    = layer;
    c->mask     = mask;
    c->ownerID  = ownerID;
    c->active   = true;
    c->dynamic  = dynamic;
    c->trigger  = trigger;

    world->colliderCount++;
    TraceLog(LOG_DEBUG, "PHYS_WORLD: Added sphere handle %d (total: %d)", h, world->colliderCount);
    return h;
}

ColliderHandle PHYS_WORLD_AddCapsule(PhysWorld* world, Vector3 position,
    float radius, float halfHeight, int layer, int mask, int ownerID,
    bool dynamic, bool trigger)
{
    assert(world);
    ColliderHandle h = FindFreeSlot(world);
    if (h == COLLIDER_HANDLE_INVALID) return h;

    Collider* c = &world->colliders[h];
    memset(c, 0, sizeof(*c));

    c->type = COLLIDER_CAPSULE;
    c->shape.capsule.radius     = radius;
    c->shape.capsule.halfHeight = halfHeight;
    c->position = position;
    c->layer    = layer;
    c->mask     = mask;
    c->ownerID  = ownerID;
    c->active   = true;
    c->dynamic  = dynamic;
    c->trigger  = trigger;

    world->colliderCount++;
    TraceLog(LOG_DEBUG, "PHYS_WORLD: Added capsule handle %d (total: %d)", h, world->colliderCount);
    return h;
}

void PHYS_WORLD_Remove(PhysWorld* world, ColliderHandle handle)
{
    assert(world);
    if (handle == COLLIDER_HANDLE_INVALID) return;
    if (handle < 0 || handle >= MAX_COLLIDERS) return;
    if (!world->colliders[handle].active) return;

    world->colliders[handle].active = false;
    world->colliderCount--;
    TraceLog(LOG_DEBUG, "PHYS_WORLD: Removed handle %d (total: %d)", handle, world->colliderCount);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Transform Updates
 * ═══════════════════════════════════════════════════════════════════════════ */

void PHYS_WORLD_SetPosition(PhysWorld* world, ColliderHandle handle, Vector3 position)
{
    assert(world);
    if (handle < 0 || handle >= MAX_COLLIDERS) return;
    if (!world->colliders[handle].active) return;

    world->colliders[handle].position = position;
}

Vector3 PHYS_WORLD_GetPosition(const PhysWorld* world, ColliderHandle handle)
{
    assert(world);
    if (handle < 0 || handle >= MAX_COLLIDERS) return (Vector3){ 0 };
    if (!world->colliders[handle].active) return (Vector3){ 0 };

    return world->colliders[handle].position;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Collision Tests — Task 2.3 (stubs)
 * ═══════════════════════════════════════════════════════════════════════════ */

bool TestSphereVsAABB(Vector3 sphereCenter, float sphereRadius,
    Vector3 boxCenter, Vector3 boxHalfExtents,
    ContactInfo* outContact)
{
    (void)sphereCenter; (void)sphereRadius;
    (void)boxCenter; (void)boxHalfExtents; (void)outContact;
    /* TODO Task 2.3 */
    return false;
}

bool TestSphereVsCapsule(Vector3 sphereCenter, float sphereRadius,
    Vector3 capsuleCenter, float capsuleRadius, float capsuleHalfHeight,
    ContactInfo* outContact)
{
    (void)sphereCenter; (void)sphereRadius;
    (void)capsuleCenter; (void)capsuleRadius; (void)capsuleHalfHeight;
    (void)outContact;
    /* TODO Task 2.3 */
    return false;
}

bool TestCapsuleVsAABB(Vector3 capsuleCenter, float capsuleRadius,
    float capsuleHalfHeight,
    Vector3 boxCenter, Vector3 boxHalfExtents,
    ContactInfo* outContact)
{
    (void)capsuleCenter; (void)capsuleRadius; (void)capsuleHalfHeight;
    (void)boxCenter; (void)boxHalfExtents; (void)outContact;
    /* TODO Task 2.3 */
    return false;
}

bool TestCapsuleVsCapsule(Vector3 centerA, float radiusA, float halfHeightA,
    Vector3 centerB, float radiusB, float halfHeightB,
    ContactInfo* outContact)
{
    (void)centerA; (void)radiusA; (void)halfHeightA;
    (void)centerB; (void)radiusB; (void)halfHeightB;
    (void)outContact;
    /* TODO Task 2.3 */
    return false;
}

bool TestRayVsAABB(Vector3 origin, Vector3 dir, float maxDist,
    Vector3 boxCenter, Vector3 boxHalfExtents,
    CollisionInfo* outHit)
{
    (void)origin; (void)dir; (void)maxDist;
    (void)boxCenter; (void)boxHalfExtents; (void)outHit;
    /* TODO Task 2.3 */
    return false;
}

bool TestRayVsCapsule(Vector3 origin, Vector3 dir, float maxDist,
    Vector3 capsuleCenter, float capsuleRadius, float capsuleHalfHeight,
    CollisionInfo* outHit)
{
    (void)origin; (void)dir; (void)maxDist;
    (void)capsuleCenter; (void)capsuleRadius; (void)capsuleHalfHeight;
    (void)outHit;
    /* TODO Task 2.3 */
    return false;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  MoveAndCollide — Task 2.4b (stub)
 * ═══════════════════════════════════════════════════════════════════════════ */

Vector3 PHYS_WORLD_MoveAndCollide(PhysWorld* world, ColliderHandle handle,
    Vector3 delta, int layerMask, bool* grounded)
{
    assert(world);
    if (handle < 0 || handle >= MAX_COLLIDERS) return (Vector3){ 0 };
    if (!world->colliders[handle].active) return (Vector3){ 0 };

    /* TODO Task 2.4b: iterative move + resolve */
    /* For now, just apply the delta directly (no collision) */
    Collider* c = &world->colliders[handle];
    c->position = Vector3Add(c->position, delta);

    if (grounded) *grounded = false;
    (void)layerMask;

    return c->position;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Per-Tick Update — Task 2.4a (stub)
 * ═══════════════════════════════════════════════════════════════════════════ */

void PHYS_WORLD_Update(PhysWorld* world,
    ContactInfo** outContacts, int* outCount)
{
    assert(world);

    world->statsPairsChecked  = 0;
    world->statsContactsFound = 0;
    world->statsTriggersFound = 0;

    /* TODO Task 2.4a: iterate pairs, check triggers, resolve passive */

    if (outContacts) *outContacts = NULL;
    if (outCount) *outCount = 0;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Queries — Task 2.5 (stubs)
 * ═══════════════════════════════════════════════════════════════════════════ */

bool PHYS_WORLD_RayCast(const PhysWorld* world, Vector3 origin, Vector3 dir,
    float maxDist, int layerMask, CollisionInfo* outHit)
{
    (void)world; (void)origin; (void)dir;
    (void)maxDist; (void)layerMask; (void)outHit;
    /* TODO Task 2.5 */
    return false;
}

bool PHYS_WORLD_RayCastIgnore(const PhysWorld* world, Vector3 origin,
    Vector3 dir, float maxDist, int layerMask,
    ColliderHandle ignore, CollisionInfo* outHit)
{
    (void)world; (void)origin; (void)dir;
    (void)maxDist; (void)layerMask; (void)ignore; (void)outHit;
    /* TODO Task 2.5 */
    return false;
}

int PHYS_WORLD_OverlapSphere(const PhysWorld* world, Vector3 center,
    float radius, int layerMask,
    ColliderHandle* outHandles, int maxResults)
{
    (void)world; (void)center; (void)radius;
    (void)layerMask; (void)outHandles; (void)maxResults;
    /* TODO Task 2.5 */
    return 0;
}

void PHYS_WORLD_DebugDraw(const PhysWorld* world)
{
    assert(world);

    for (int i = 0; i < MAX_COLLIDERS; i++)
    {
        const Collider* c = &world->colliders[i];
        if (!c->active) continue;

        /* Pick color by type */
        Color color;
        if (c->trigger)       color = RED;
        else if (c->dynamic)  color = YELLOW;
        else                  color = GREEN;

        switch (c->type)
        {
            case COLLIDER_AABB:
            {
                Vector3 size = {
                    c->shape.box.halfExtents.x * 2.0f,
                    c->shape.box.halfExtents.y * 2.0f,
                    c->shape.box.halfExtents.z * 2.0f,
                };
                DrawCubeWiresV(c->position, size, color);
                break;
            }

            case COLLIDER_SPHERE:
            {
                DrawSphereWires(c->position, c->shape.sphere.radius, 8, 8, color);
                break;
            }

            case COLLIDER_CAPSULE:
            {
                /* Raylib's DrawCapsuleWires takes the two hemisphere centers */
                float hh = c->shape.capsule.halfHeight;
                Vector3 top = { c->position.x, c->position.y + hh, c->position.z };
                Vector3 bot = { c->position.x, c->position.y - hh, c->position.z };
                DrawCapsuleWires(bot, top, c->shape.capsule.radius, 8, 4, color);
                break;
            }
        }
    }
}