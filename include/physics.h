#pragma once

#include "physics_types.h"

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
