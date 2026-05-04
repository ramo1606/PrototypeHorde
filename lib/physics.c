#include "physics.h"
#include "raylib.h"
#include "raymath.h"

#include <assert.h>
#include <string.h>
#include <math.h>

/*******************************************************************************************
*
*   physics.c — Physics World: Colliders, Detection, and Queries
*
*   Task 2.1: Structs defined in physics.h ✓
*   Task 2.2: Collider pool, registration, position updates. ✓
*   Task 2.3: 6 collision test functions (pure geometry). ✓
*   Task 2.4: PhysicsUpdate + MoveAndCollide. ✓
*   Task 2.5: Raycast and overlap queries. ✓
*   Task 2.6: Debug wireframe drawing + panel F4. ✓
*
********************************************************************************************/

/* ═══════════════════════════════════════════════════════════════════════════
 *  Lifecycle
 * ═══════════════════════════════════════════════════════════════════════════ */

void PhysicsInit(PhysWorld* world)
{
    assert(world);
    memset(world, 0, sizeof(*world));
    TraceLog(LOG_INFO, "Physics: Initialized (max %d colliders)", MAX_COLLIDERS);
}

void PhysicsShutdown(PhysWorld* world)
{
    assert(world);
    memset(world->colliders, 0, sizeof(world->colliders));
    world->colliderCount = 0;
    TraceLog(LOG_INFO, "Physics: Shutdown");
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Registration
 * ═══════════════════════════════════════════════════════════════════════════ */

static ColliderHandle FindFreeSlot(PhysWorld* world)
{
    for (int i = 0; i < MAX_COLLIDERS; i++)
    {
        if (!world->colliders[i].active) return i;
    }
    TraceLog(LOG_WARNING, "Physics: Pool full (max %d)", MAX_COLLIDERS);
    return COLLIDER_HANDLE_INVALID;
}

ColliderHandle PhysicsAddBox(PhysWorld* world, Vector3 position,
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
    c->layer = layer;  c->mask = mask;  c->ownerID = ownerID;
    c->active = true;  c->dynamic = dynamic;  c->trigger = trigger;

    world->colliderCount++;
    TraceLog(LOG_DEBUG, "Physics: Added box handle %d (total: %d)", h, world->colliderCount);
    return h;
}

ColliderHandle PhysicsAddSphere(PhysWorld* world, Vector3 position,
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
    c->layer = layer;  c->mask = mask;  c->ownerID = ownerID;
    c->active = true;  c->dynamic = dynamic;  c->trigger = trigger;

    world->colliderCount++;
    TraceLog(LOG_DEBUG, "Physics: Added sphere handle %d (total: %d)", h, world->colliderCount);
    return h;
}

ColliderHandle PhysicsAddCapsule(PhysWorld* world, Vector3 position,
    float radius, float halfHeight, int layer, int mask, int ownerID,
    bool dynamic, bool trigger)
{
    assert(world);
    ColliderHandle h = FindFreeSlot(world);
    if (h == COLLIDER_HANDLE_INVALID) return h;

    Collider* c = &world->colliders[h];
    memset(c, 0, sizeof(*c));
    c->type = COLLIDER_CAPSULE;
    c->shape.capsule.radius = radius;
    c->shape.capsule.halfHeight = halfHeight;
    c->position = position;
    c->layer = layer;  c->mask = mask;  c->ownerID = ownerID;
    c->active = true;  c->dynamic = dynamic;  c->trigger = trigger;

    world->colliderCount++;
    TraceLog(LOG_DEBUG, "Physics: Added capsule handle %d (total: %d)", h, world->colliderCount);
    return h;
}

void PhysicsRemove(PhysWorld* world, ColliderHandle handle)
{
    assert(world);
    if (handle == COLLIDER_HANDLE_INVALID) return;
    if (handle < 0 || handle >= MAX_COLLIDERS) return;
    if (!world->colliders[handle].active) return;

    world->colliders[handle].active = false;
    world->colliderCount--;
    TraceLog(LOG_DEBUG, "Physics: Removed handle %d (total: %d)", handle, world->colliderCount);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Transform Updates
 * ═══════════════════════════════════════════════════════════════════════════ */

void PhysicsSetPosition(PhysWorld* world, ColliderHandle handle, Vector3 position)
{
    assert(world);
    if (handle < 0 || handle >= MAX_COLLIDERS) return;
    if (!world->colliders[handle].active) return;
    world->colliders[handle].position = position;
}

Vector3 PhysicsGetPosition(const PhysWorld* world, ColliderHandle handle)
{
    assert(world);
    if (handle < 0 || handle >= MAX_COLLIDERS) return (Vector3) { 0 };
    if (!world->colliders[handle].active) return (Vector3) { 0 };
    return world->colliders[handle].position;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Collision Tests — Task 2.3
 *
 *  Pure geometry functions. No state, no side effects.
 *  Convention: contact normal points FROM B TOWARD A.
 *  Uses raylib/raymath: Clamp, Vector3LengthSqr, Vector3Length,
 *  Vector3Normalize, GetRayCollisionBox, GetRayCollisionSphere, etc.
 * ═══════════════════════════════════════════════════════════════════════════ */

 /* ── Helpers ──────────────────────────────────────────────────────────────── */

static Vector3 ClosestPointOnSegmentY(Vector3 segCenter, float halfHeight, Vector3 point)
{
    float clampedY = Clamp(point.y, segCenter.y - halfHeight, segCenter.y + halfHeight);
    return (Vector3) { segCenter.x, clampedY, segCenter.z };
}

static Vector3 ClosestPointOnAABB(Vector3 boxCenter, Vector3 boxHalf, Vector3 point)
{
    return (Vector3) {
        Clamp(point.x, boxCenter.x - boxHalf.x, boxCenter.x + boxHalf.x),
            Clamp(point.y, boxCenter.y - boxHalf.y, boxCenter.y + boxHalf.y),
            Clamp(point.z, boxCenter.z - boxHalf.z, boxCenter.z + boxHalf.z),
    };
}

/* ── 1. Sphere vs AABB ───────────────────────────────────────────────────── */

bool TestSphereVsAABB(Vector3 sphereCenter, float sphereRadius,
    Vector3 boxCenter, Vector3 boxHalf,
    ContactInfo* out)
{
    Vector3 closest = ClosestPointOnAABB(boxCenter, boxHalf, sphereCenter);
    Vector3 delta = Vector3Subtract(sphereCenter, closest);
    float distSq = Vector3LengthSqr(delta);

    bool inside = (sphereCenter.x >= boxCenter.x - boxHalf.x &&
        sphereCenter.x <= boxCenter.x + boxHalf.x &&
        sphereCenter.y >= boxCenter.y - boxHalf.y &&
        sphereCenter.y <= boxCenter.y + boxHalf.y &&
        sphereCenter.z >= boxCenter.z - boxHalf.z &&
        sphereCenter.z <= boxCenter.z + boxHalf.z);

    if (inside)
    {
        float dx = boxHalf.x - fabsf(sphereCenter.x - boxCenter.x);
        float dy = boxHalf.y - fabsf(sphereCenter.y - boxCenter.y);
        float dz = boxHalf.z - fabsf(sphereCenter.z - boxCenter.z);

        if (out)
        {
            out->hit = true;
            out->contactPoint = closest;
            if (dx <= dy && dx <= dz) {
                out->normal = (Vector3){ (sphereCenter.x > boxCenter.x) ? 1.0f : -1.0f, 0, 0 };
                out->penetration = dx + sphereRadius;
            }
            else if (dy <= dz) {
                out->normal = (Vector3){ 0, (sphereCenter.y > boxCenter.y) ? 1.0f : -1.0f, 0 };
                out->penetration = dy + sphereRadius;
            }
            else {
                out->normal = (Vector3){ 0, 0, (sphereCenter.z > boxCenter.z) ? 1.0f : -1.0f };
                out->penetration = dz + sphereRadius;
            }
        }
        return true;
    }

    if (distSq > sphereRadius * sphereRadius) return false;

    float dist = Vector3Length(delta);
    if (out)
    {
        out->hit = true;
        out->penetration = sphereRadius - dist;
        out->contactPoint = closest;
        out->normal = (dist > 0.0001f) ? Vector3Normalize(delta) : (Vector3) { 0, 1, 0 };
    }
    return true;
}

/* ── 2. Sphere vs Capsule ────────────────────────────────────────────────── */

bool TestSphereVsCapsule(Vector3 sphereCenter, float sphereRadius,
    Vector3 capsuleCenter, float capsuleRadius, float capsuleHalfHeight,
    ContactInfo* out)
{
    Vector3 closest = ClosestPointOnSegmentY(capsuleCenter, capsuleHalfHeight, sphereCenter);
    Vector3 delta = Vector3Subtract(sphereCenter, closest);
    float distSq = Vector3LengthSqr(delta);
    float combinedR = sphereRadius + capsuleRadius;

    if (distSq > combinedR * combinedR) return false;

    float dist = Vector3Length(delta);
    if (out)
    {
        out->hit = true;
        out->penetration = combinedR - dist;
        out->normal = (dist > 0.0001f) ? Vector3Normalize(delta) : (Vector3) { 0, 1, 0 };
        out->contactPoint = Vector3Add(closest, Vector3Scale(out->normal, capsuleRadius));
    }
    return true;
}

/* ── 3. Capsule vs AABB ──────────────────────────────────────────────────── */

bool TestCapsuleVsAABB(Vector3 capsuleCenter, float capsuleRadius,
    float capsuleHalfHeight,
    Vector3 boxCenter, Vector3 boxHalf,
    ContactInfo* out)
{
    float segTop = capsuleCenter.y + capsuleHalfHeight;
    float segBot = capsuleCenter.y - capsuleHalfHeight;
    float boxTop = boxCenter.y + boxHalf.y;
    float boxBot = boxCenter.y - boxHalf.y;

    float bestY;
    if (segBot > boxTop)        bestY = segBot;
    else if (segTop < boxBot)   bestY = segTop;
    else                        bestY = Clamp(boxCenter.y, segBot, segTop);

    Vector3 segPoint = { capsuleCenter.x, bestY, capsuleCenter.z };
    return TestSphereVsAABB(segPoint, capsuleRadius, boxCenter, boxHalf, out);
}

/* ── 4. Capsule vs Capsule ───────────────────────────────────────────────── */

bool TestCapsuleVsCapsule(Vector3 centerA, float radiusA, float halfHeightA,
    Vector3 centerB, float radiusB, float halfHeightB,
    ContactInfo* out)
{
    float topA = centerA.y + halfHeightA, botA = centerA.y - halfHeightA;
    float topB = centerB.y + halfHeightB, botB = centerB.y - halfHeightB;

    /* Closest points on two parallel (vertical) segments.
     * If the Y-intervals overlap, both closest points share the midpoint
     * of the overlap. Otherwise they're the nearest endpoints. */
    float yA, yB;
    float yLow  = (botA > botB) ? botA : botB;
    float yHigh = (topA < topB) ? topA : topB;
    if (yLow <= yHigh)
    {
        yA = yB = (yLow + yHigh) * 0.5f;
    }
    else if (topA < botB)
    {
        yA = topA;  yB = botB;
    }
    else
    {
        yA = botA;  yB = topB;
    }

    Vector3 ptA = { centerA.x, yA, centerA.z };
    Vector3 ptB = { centerB.x, yB, centerB.z };
    Vector3 delta = Vector3Subtract(ptA, ptB);
    float distSq = Vector3LengthSqr(delta);
    float combinedR = radiusA + radiusB;

    if (distSq > combinedR * combinedR) return false;

    float dist = Vector3Length(delta);
    if (out)
    {
        out->hit = true;
        out->penetration = combinedR - dist;
        out->normal = (dist > 0.0001f) ? Vector3Normalize(delta) : (Vector3) { 1, 0, 0 };
        out->contactPoint = Vector3Add(ptB, Vector3Scale(out->normal, radiusB));
    }
    return true;
}

/* ── 5. Ray vs AABB (delegates to raylib) ────────────────────────────────── */

bool TestRayVsAABB(Vector3 origin, Vector3 dir, float maxDist,
    Vector3 boxCenter, Vector3 boxHalf,
    CollisionInfo* out)
{
    Ray ray = { origin, dir };
    BoundingBox box = {
        Vector3Subtract(boxCenter, boxHalf),
        Vector3Add(boxCenter, boxHalf),
    };

    RayCollision rc = GetRayCollisionBox(ray, box);

    if (!rc.hit || rc.distance < 0.0f || rc.distance > maxDist) return false;

    if (out)
    {
        out->hit = true;
        out->distance = rc.distance;
        out->point = rc.point;
        out->normal = rc.normal;
    }
    return true;
}

/* ── 6. Ray vs Capsule ───────────────────────────────────────────────────── */
/*
 * Cylinder shaft: project to XZ, solve 2D ray-circle quadratic.
 * Hemisphere caps: delegate to raylib's GetRayCollisionSphere.
 * Take the closest valid hit.
 */
bool TestRayVsCapsule(Vector3 origin, Vector3 dir, float maxDist,
    Vector3 capsuleCenter, float capsuleRadius, float capsuleHalfHeight,
    CollisionInfo* out)
{
    float bestDist = maxDist + 1.0f;
    Vector3 bestPoint = { 0 };
    Vector3 bestNormal = { 0 };
    bool anyHit = false;

    float r = capsuleRadius;
    float hh = capsuleHalfHeight;
    Ray ray = { origin, dir };

    /* ── Cylinder shaft (XZ projection, quadratic) ────────────────────── */
    {
        float ox = origin.x - capsuleCenter.x;
        float oz = origin.z - capsuleCenter.z;

        float a = dir.x * dir.x + dir.z * dir.z;
        float b = 2.0f * (ox * dir.x + oz * dir.z);
        float c = ox * ox + oz * oz - r * r;

        if (a > 0.000001f)
        {
            float disc = b * b - 4.0f * a * c;
            if (disc >= 0.0f)
            {
                float sqrtDisc = sqrtf(disc);
                float t = (-b - sqrtDisc) / (2.0f * a);
                if (t < 0.0f) t = (-b + sqrtDisc) / (2.0f * a);

                if (t >= 0.0f && t <= maxDist)
                {
                    Vector3 hitPt = Vector3Add(origin, Vector3Scale(dir, t));

                    if (hitPt.y >= capsuleCenter.y - hh && hitPt.y <= capsuleCenter.y + hh)
                    {
                        if (t < bestDist)
                        {
                            bestDist = t;
                            bestPoint = hitPt;
                            Vector3 outward = { hitPt.x - capsuleCenter.x, 0.0f, hitPt.z - capsuleCenter.z };
                            bestNormal = Vector3Normalize(outward);
                            anyHit = true;
                        }
                    }
                }
            }
        }
    }

    /* ── Hemisphere caps (via raylib) ─────────────────────────────────── */
    Vector3 capCenters[2] = {
        { capsuleCenter.x, capsuleCenter.y + hh, capsuleCenter.z },
        { capsuleCenter.x, capsuleCenter.y - hh, capsuleCenter.z },
    };

    for (int i = 0; i < 2; i++)
    {
        RayCollision rc = GetRayCollisionSphere(ray, capCenters[i], r);

        if (!rc.hit || rc.distance < 0.0f || rc.distance > maxDist) continue;
        if (rc.distance >= bestDist) continue;

        /* Only count the correct hemisphere half */
        float hitRelY = rc.point.y - capCenters[i].y;
        if (i == 0 && hitRelY < 0.0f) continue;   /* Top: upper half only */
        if (i == 1 && hitRelY > 0.0f) continue;   /* Bottom: lower half only */

        bestDist = rc.distance;
        bestPoint = rc.point;
        bestNormal = rc.normal;
        anyHit = true;
    }

    if (anyHit && out)
    {
        out->hit = true;
        out->distance = bestDist;
        out->point = bestPoint;
        out->normal = bestNormal;
    }
    return anyHit;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Collision Dispatcher
 *
 *  Given two colliders, picks the correct test function based on their types.
 *  Returns true if overlapping. The contact normal points FROM B TOWARD A.
 * ═══════════════════════════════════════════════════════════════════════════ */

static bool TestColliderPair(const Collider* a, const Collider* b, ContactInfo* out)
{
    /* Sphere vs AABB */
    if (a->type == COLLIDER_SPHERE && b->type == COLLIDER_AABB)
        return TestSphereVsAABB(a->position, a->shape.sphere.radius,
            b->position, b->shape.box.halfExtents, out);

    if (a->type == COLLIDER_AABB && b->type == COLLIDER_SPHERE)
    {
        bool hit = TestSphereVsAABB(b->position, b->shape.sphere.radius,
            a->position, a->shape.box.halfExtents, out);
        if (hit && out) out->normal = Vector3Negate(out->normal);
        return hit;
    }

    /* Sphere vs Capsule */
    if (a->type == COLLIDER_SPHERE && b->type == COLLIDER_CAPSULE)
        return TestSphereVsCapsule(a->position, a->shape.sphere.radius,
            b->position, b->shape.capsule.radius, b->shape.capsule.halfHeight, out);

    if (a->type == COLLIDER_CAPSULE && b->type == COLLIDER_SPHERE)
    {
        bool hit = TestSphereVsCapsule(b->position, b->shape.sphere.radius,
            a->position, a->shape.capsule.radius, a->shape.capsule.halfHeight, out);
        if (hit && out) out->normal = Vector3Negate(out->normal);
        return hit;
    }

    /* Capsule vs AABB */
    if (a->type == COLLIDER_CAPSULE && b->type == COLLIDER_AABB)
        return TestCapsuleVsAABB(a->position, a->shape.capsule.radius,
            a->shape.capsule.halfHeight,
            b->position, b->shape.box.halfExtents, out);

    if (a->type == COLLIDER_AABB && b->type == COLLIDER_CAPSULE)
    {
        bool hit = TestCapsuleVsAABB(b->position, b->shape.capsule.radius,
            b->shape.capsule.halfHeight,
            a->position, a->shape.box.halfExtents, out);
        if (hit && out) out->normal = Vector3Negate(out->normal);
        return hit;
    }

    /* Capsule vs Capsule */
    if (a->type == COLLIDER_CAPSULE && b->type == COLLIDER_CAPSULE)
        return TestCapsuleVsCapsule(
            a->position, a->shape.capsule.radius, a->shape.capsule.halfHeight,
            b->position, b->shape.capsule.radius, b->shape.capsule.halfHeight, out);

    /* Sphere vs Sphere */
    if (a->type == COLLIDER_SPHERE && b->type == COLLIDER_SPHERE)
    {
        /* Reuse SphereVsCapsule with zero halfHeight */
        return TestSphereVsCapsule(a->position, a->shape.sphere.radius,
            b->position, b->shape.sphere.radius, 0.0f, out);
    }

    /* AABB vs AABB — not needed for gameplay (both static), but safe fallback */
    return false;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  MoveAndCollide — Task 2.4b
 *
 *  Moves a dynamic collider by delta, then iteratively resolves
 *  penetrations against all matching colliders.
 *
 *  Algorithm:
 *    1. Apply delta to position
 *    2. For each iteration (up to MOVE_AND_COLLIDE_ITERATIONS):
 *       a. Find the deepest penetrating contact
 *       b. Push position out along contact normal
 *       c. Track if any contact is ground (normal.y > threshold)
 *    3. Write final position back to the collider
 *    4. Return final position
 *
 *  Why find deepest first? If we resolve in penetration order, the
 *  biggest overlap gets fixed first, and smaller overlaps often resolve
 *  themselves as a side effect. With 3 iterations this handles corners
 *  (two walls meeting) correctly.
 * ═══════════════════════════════════════════════════════════════════════════ */

#define GROUND_NORMAL_THRESHOLD 0.7f    /* normal.y > this = ground contact */

Vector3 PhysicsMoveAndCollide(PhysWorld* world, ColliderHandle handle,
    Vector3 delta, int layerMask, bool* grounded)
{
    assert(world);
    if (handle < 0 || handle >= MAX_COLLIDERS) return (Vector3) { 0 };
    if (!world->colliders[handle].active) return (Vector3) { 0 };

    Collider* mover = &world->colliders[handle];
    if (grounded) *grounded = false;

    /* Apply the movement delta */
    mover->position = Vector3Add(mover->position, delta);

    /* Iterative resolution */
    for (int iter = 0; iter < MOVE_AND_COLLIDE_ITERATIONS; iter++)
    {
        /* Find the deepest penetrating contact */
        float deepestPen = 0.0f;
        Vector3 deepestNormal = { 0 };
        bool foundContact = false;

        for (int i = 0; i < MAX_COLLIDERS; i++)
        {
            if (i == handle) continue;

            const Collider* other = &world->colliders[i];
            if (!other->active) continue;

            /* Layer/mask check: both must agree to interact */
            if (!(mover->mask & other->layer & layerMask)) continue;
            if (!(other->mask & mover->layer)) continue;

            /* Skip triggers — they detect but don't push */
            if (other->trigger) continue;

            ContactInfo contact = { 0 };
            if (TestColliderPair(mover, other, &contact))
            {
                if (contact.penetration > deepestPen)
                {
                    deepestPen = contact.penetration;
                    deepestNormal = contact.normal;
                    foundContact = true;
                }

                /* Check for ground contact */
                if (grounded && contact.normal.y > GROUND_NORMAL_THRESHOLD)
                {
                    *grounded = true;
                }
            }
        }

        if (!foundContact) break;   /* No more overlaps — done */

        /* Push out along the deepest contact normal */
        mover->position = Vector3Add(mover->position,
            Vector3Scale(deepestNormal, deepestPen));
    }

    return mover->position;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Per-Tick Update — Task 2.4a
 *
 *  Processes passive interactions that MoveAndCollide doesn't handle:
 *    - Trigger overlaps (pickup collection, mine detonation, zone entry)
 *
 *  Iterates all unique pairs, checks layer compatibility, tests overlap.
 *  Trigger contacts are collected into a static array that the caller
 *  can read (outContacts/outCount).
 *
 *  This runs AFTER gameplay has called MoveAndCollide for its entities,
 *  so all positions are up to date.
 * ═══════════════════════════════════════════════════════════════════════════ */

#define MAX_TRIGGER_CONTACTS 64

static ContactInfo s_triggerContacts[MAX_TRIGGER_CONTACTS];

void PhysicsUpdate(PhysWorld* world,
    ContactInfo** outContacts, int* outCount)
{
    assert(world);

    world->statsPairsChecked = 0;
    world->statsContactsFound = 0;
    world->statsTriggersFound = 0;

    int triggerCount = 0;

    /* Iterate all unique pairs (i < j) */
    for (int i = 0; i < MAX_COLLIDERS; i++)
    {
        const Collider* a = &world->colliders[i];
        if (!a->active) continue;

        for (int j = i + 1; j < MAX_COLLIDERS; j++)
        {
            const Collider* b = &world->colliders[j];
            if (!b->active) continue;

            /* Layer/mask compatibility: both must agree */
            if (!(a->mask & b->layer)) continue;
            if (!(b->mask & a->layer)) continue;

            /* Only process pairs where at least one is a trigger */
            if (!a->trigger && !b->trigger) continue;

            world->statsPairsChecked++;

            ContactInfo contact = { 0 };
            if (TestColliderPair(a, b, &contact))
            {
                world->statsTriggersFound++;

                contact.colliderA = i;
                contact.colliderB = j;
                contact.ownerA = a->ownerID;
                contact.ownerB = b->ownerID;

                if (triggerCount < MAX_TRIGGER_CONTACTS)
                {
                    s_triggerContacts[triggerCount++] = contact;
                }
            }
        }
    }

    if (outContacts) *outContacts = (triggerCount > 0) ? s_triggerContacts : NULL;
    if (outCount) *outCount = triggerCount;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Queries — Task 2.5
 *
 *  On-demand queries called by gameplay. Not automatic — the caller
 *  decides when to use them.
 *
 *  RayCast:       shooting, line-of-sight checks, camera collision
 *  RayCastIgnore: shooting (ignore the shooter's own collider)
 *  OverlapSphere: AOE damage (explosions, grenades, mine radius)
 * ═══════════════════════════════════════════════════════════════════════════ */

 /*
  * Internal: raycast a single collider. Returns true if hit, fills outHit.
  */
static bool RayCastSingle(const Collider* c, Vector3 origin, Vector3 dir,
    float maxDist, CollisionInfo* outHit)
{
    switch (c->type)
    {
    case COLLIDER_AABB:
        return TestRayVsAABB(origin, dir, maxDist,
            c->position, c->shape.box.halfExtents, outHit);

    case COLLIDER_SPHERE:
    {
        /* Use raylib directly */
        Ray ray = { origin, dir };
        RayCollision rc = GetRayCollisionSphere(ray, c->position, c->shape.sphere.radius);
        if (!rc.hit || rc.distance < 0.0f || rc.distance > maxDist) return false;
        if (outHit)
        {
            outHit->hit = true;
            outHit->distance = rc.distance;
            outHit->point = rc.point;
            outHit->normal = rc.normal;
        }
        return true;
    }

    case COLLIDER_CAPSULE:
        return TestRayVsCapsule(origin, dir, maxDist,
            c->position, c->shape.capsule.radius,
            c->shape.capsule.halfHeight, outHit);
    }
    return false;
}

/*
 * Internal: test if a sphere overlaps a single collider.
 */
static bool OverlapSphereSingle(const Collider* c, Vector3 center, float radius)
{
    switch (c->type)
    {
    case COLLIDER_AABB:
        return TestSphereVsAABB(center, radius,
            c->position, c->shape.box.halfExtents, NULL);

    case COLLIDER_SPHERE:
    {
        float combinedR = radius + c->shape.sphere.radius;
        float distSq = Vector3LengthSqr(Vector3Subtract(center, c->position));
        return distSq <= combinedR * combinedR;
    }

    case COLLIDER_CAPSULE:
        return TestSphereVsCapsule(center, radius,
            c->position, c->shape.capsule.radius,
            c->shape.capsule.halfHeight, NULL);
    }
    return false;
}

bool PhysicsRayCast(const PhysWorld* world, Vector3 origin, Vector3 dir,
    float maxDist, int layerMask, CollisionInfo* outHit)
{
    assert(world);

    float closestDist = maxDist + 1.0f;
    CollisionInfo closestHit = { 0 };
    int closestHandle = COLLIDER_HANDLE_INVALID;

    for (int i = 0; i < MAX_COLLIDERS; i++)
    {
        const Collider* c = &world->colliders[i];
        if (!c->active) continue;
        if (!(c->layer & layerMask)) continue;

        CollisionInfo hit = { 0 };
        if (RayCastSingle(c, origin, dir, maxDist, &hit))
        {
            if (hit.distance < closestDist)
            {
                closestDist = hit.distance;
                closestHit = hit;
                closestHandle = i;
            }
        }
    }

    if (closestHandle == COLLIDER_HANDLE_INVALID) return false;

    if (outHit)
    {
        *outHit = closestHit;
        outHit->collider = closestHandle;
        outHit->ownerID = world->colliders[closestHandle].ownerID;
    }
    return true;
}

bool PhysicsRayCastIgnore(const PhysWorld* world, Vector3 origin,
    Vector3 dir, float maxDist, int layerMask,
    ColliderHandle ignore, CollisionInfo* outHit)
{
    assert(world);

    float closestDist = maxDist + 1.0f;
    CollisionInfo closestHit = { 0 };
    int closestHandle = COLLIDER_HANDLE_INVALID;

    for (int i = 0; i < MAX_COLLIDERS; i++)
    {
        if (i == ignore) continue;

        const Collider* c = &world->colliders[i];
        if (!c->active) continue;
        if (!(c->layer & layerMask)) continue;

        CollisionInfo hit = { 0 };
        if (RayCastSingle(c, origin, dir, maxDist, &hit))
        {
            if (hit.distance < closestDist)
            {
                closestDist = hit.distance;
                closestHit = hit;
                closestHandle = i;
            }
        }
    }

    if (closestHandle == COLLIDER_HANDLE_INVALID) return false;

    if (outHit)
    {
        *outHit = closestHit;
        outHit->collider = closestHandle;
        outHit->ownerID = world->colliders[closestHandle].ownerID;
    }
    return true;
}

int PhysicsOverlapSphere(const PhysWorld* world, Vector3 center,
    float radius, int layerMask,
    ColliderHandle* outHandles, int maxResults)
{
    assert(world);

    int count = 0;

    for (int i = 0; i < MAX_COLLIDERS; i++)
    {
        if (count >= maxResults) break;

        const Collider* c = &world->colliders[i];
        if (!c->active) continue;
        if (!(c->layer & layerMask)) continue;

        if (OverlapSphereSingle(c, center, radius))
        {
            outHandles[count++] = i;
        }
    }

    return count;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Debug Drawing (Task 2.6)
 * ═══════════════════════════════════════════════════════════════════════════ */

void PhysicsDebugDraw(const PhysWorld* world)
{
    assert(world);

    for (int i = 0; i < MAX_COLLIDERS; i++)
    {
        const Collider* c = &world->colliders[i];
        if (!c->active) continue;

        Color color;
        if (c->trigger)       color = RED;
        else if (c->dynamic)  color = YELLOW;
        else                  color = GREEN;

        switch (c->type)
        {
        case COLLIDER_AABB:
        {
            Vector3 size = Vector3Scale(c->shape.box.halfExtents, 2.0f);
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
            float hh = c->shape.capsule.halfHeight;
            Vector3 top = { c->position.x, c->position.y + hh, c->position.z };
            Vector3 bot = { c->position.x, c->position.y - hh, c->position.z };
            DrawCapsuleWires(bot, top, c->shape.capsule.radius, 8, 4, color);
            break;
        }
        }
    }
}