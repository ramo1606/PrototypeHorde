#pragma once

/*
 * physics.h — Collider pool, collision tests, character movement,
 *             raycast / overlap queries.
 *
 * DEPENDENCIES: raylib.h
 * OVERRIDES (define before include):
 *   MAX_COLLIDERS               (default 256)
 *   MOVE_AND_COLLIDE_ITERATIONS (default 3)
 *
 * Layers and masks are plain `int` bitfields. The module does not name
 * any specific layers — define your own (e.g. LAYER_PLAYER = (1<<0))
 * in your project. Two colliders test only when
 * (A.mask & B.layer) && (B.mask & A.layer).
 */

#include "raylib.h"
#include <stdbool.h>

#ifndef MAX_COLLIDERS
#  define MAX_COLLIDERS 256
#endif

#ifndef MOVE_AND_COLLIDE_ITERATIONS
#  define MOVE_AND_COLLIDE_ITERATIONS 3
#endif

typedef int ColliderHandle;
#define COLLIDER_HANDLE_INVALID (-1)

typedef enum
{
    COLLIDER_AABB,
    COLLIDER_SPHERE,
    COLLIDER_CAPSULE,
} ColliderType;

/* ── Shape data ──────────────────────────────────────────────────────────── */
/* All shapes are centered on the collider's `position`. */

typedef struct ColliderAABB
{
    Vector3 halfExtents;
} ColliderAABB;

typedef struct ColliderSphere
{
    float radius;
} ColliderSphere;

/* Capsule: cylinder with hemispherical caps, oriented along Y. The line
 * segment runs from `position.y ± halfHeight`, with `radius` extending
 * from every point on it. Total height = 2*halfHeight + 2*radius. */
typedef struct ColliderCapsule
{
    float radius;
    float halfHeight;
} ColliderCapsule;

typedef union
{
    ColliderAABB    box;
    ColliderSphere  sphere;
    ColliderCapsule capsule;
} ColliderShape;

typedef struct Collider
{
    ColliderType   type;
    ColliderShape  shape;

    Vector3 position;       /* World-space center */

    int layer;              /* What this collider IS (single bit) */
    int mask;               /* What this collider hits (bits)     */

    int  ownerID;           /* Gameplay-defined; physics doesn't care.
                            * Convention: -1 = scenery / no owner. */

    bool active;
    bool dynamic;           /* Movable via MoveAndCollide?         */
    bool trigger;           /* Detect overlap but don't resolve?   */
} Collider;

/* ── Collision results ───────────────────────────────────────────────────── */

/* Result of a raycast or point query. */
typedef struct CollisionInfo
{
    bool           hit;
    float          distance;
    Vector3        point;
    Vector3        normal;
    ColliderHandle collider;
    int            ownerID;
} CollisionInfo;

/* Result of a body-body overlap. Normal points FROM B TOWARD A; pushing
 * A along normal * penetration resolves the overlap. */
typedef struct ContactInfo
{
    bool           hit;
    float          penetration;
    Vector3        normal;
    Vector3        contactPoint;

    ColliderHandle colliderA;
    ColliderHandle colliderB;
    int            ownerA;
    int            ownerB;
} ContactInfo;

typedef struct PhysWorld
{
    Collider colliders[MAX_COLLIDERS];
    int      colliderCount;

    /* Per-frame stats */
    int statsPairsChecked;
    int statsContactsFound;
    int statsTriggersFound;
} PhysWorld;

/* ── Lifecycle ───────────────────────────────────────────────────────────── */

void PhysicsInit(PhysWorld* world);
void PhysicsShutdown(PhysWorld* world);

/* ── Registration ────────────────────────────────────────────────────────── */

ColliderHandle PhysicsAddBox(PhysWorld* world, Vector3 position,
                             Vector3 halfExtents, int layer, int mask, int ownerID,
                             bool dynamic, bool trigger);

ColliderHandle PhysicsAddSphere(PhysWorld* world, Vector3 position,
                                float radius, int layer, int mask, int ownerID,
                                bool dynamic, bool trigger);

ColliderHandle PhysicsAddCapsule(PhysWorld* world, Vector3 position,
                                 float radius, float halfHeight,
                                 int layer, int mask, int ownerID,
                                 bool dynamic, bool trigger);

void PhysicsRemove(PhysWorld* world, ColliderHandle handle);

/* ── Transform Updates ───────────────────────────────────────────────────── */

void    PhysicsSetPosition(PhysWorld* world, ColliderHandle handle, Vector3 position);
Vector3 PhysicsGetPosition(const PhysWorld* world, ColliderHandle handle);

/* ── Movement with Collision Resolution ──────────────────────────────────── */

/* Move a dynamic collider by `delta`, resolving collisions iteratively
 * (up to MOVE_AND_COLLIDE_ITERATIONS). Returns the resolved position.
 * `grounded` (optional) is set true if any contact had a mostly-upward
 * normal (floor contact). */
Vector3 PhysicsMoveAndCollide(PhysWorld* world, ColliderHandle handle,
                              Vector3 delta, int layerMask, bool* grounded);

/* ── Per-Tick Update ─────────────────────────────────────────────────────── */

/* Process passive interactions: trigger overlaps, dynamic-vs-dynamic pairs
 * that weren't moved with MoveAndCollide this tick. `outContacts`/`outCount`
 * receive the trigger contacts (may be NULL if not needed). */
void PhysicsUpdate(PhysWorld* world,
                   ContactInfo** outContacts, int* outCount);

/* ── Queries ─────────────────────────────────────────────────────────────── */

bool PhysicsRayCast(const PhysWorld* world, Vector3 origin, Vector3 dir,
                    float maxDist, int layerMask, CollisionInfo* outHit);

/* Same as RayCast but ignores one specific collider handle. */
bool PhysicsRayCastIgnore(const PhysWorld* world, Vector3 origin, Vector3 dir,
                          float maxDist, int layerMask,
                          ColliderHandle ignore, CollisionInfo* outHit);

/* All colliders overlapping a sphere. Returns count written to outHandles
 * (capped at maxResults). */
int PhysicsOverlapSphere(const PhysWorld* world, Vector3 center, float radius,
                         int layerMask,
                         ColliderHandle* outHandles, int maxResults);

/* ── Pure Tests (no PhysWorld state) ─────────────────────────────────────── */
/* Contact normal points FROM B TOWARD A. */

bool TestSphereVsAABB(Vector3 sphereCenter, float sphereRadius,
                      Vector3 boxCenter, Vector3 boxHalfExtents,
                      ContactInfo* outContact);

bool TestSphereVsCapsule(Vector3 sphereCenter, float sphereRadius,
                         Vector3 capsuleCenter, float capsuleRadius, float capsuleHalfHeight,
                         ContactInfo* outContact);

bool TestCapsuleVsAABB(Vector3 capsuleCenter, float capsuleRadius, float capsuleHalfHeight,
                       Vector3 boxCenter, Vector3 boxHalfExtents,
                       ContactInfo* outContact);

bool TestCapsuleVsCapsule(Vector3 centerA, float radiusA, float halfHeightA,
                          Vector3 centerB, float radiusB, float halfHeightB,
                          ContactInfo* outContact);

bool TestRayVsAABB(Vector3 origin, Vector3 dir, float maxDist,
                   Vector3 boxCenter, Vector3 boxHalfExtents,
                   CollisionInfo* outHit);

bool TestRayVsCapsule(Vector3 origin, Vector3 dir, float maxDist,
                      Vector3 capsuleCenter, float capsuleRadius, float capsuleHalfHeight,
                      CollisionInfo* outHit);

/* ── Debug Drawing ───────────────────────────────────────────────────────── */
/* Wireframes for all active colliders. Call inside BeginMode3D.
 * Colors: green = static, yellow = dynamic, red = trigger. */
void PhysicsDebugDraw(const PhysWorld* world);
