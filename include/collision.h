#pragma once

#include "raylib.h"
#include "raymath.h"

BoundingBox COLLISION_TransformAABB(BoundingBox local, Matrix transform);
BoundingBox COLLISION_MergeAABB(BoundingBox a, BoundingBox b);
BoundingBox COLLISION_ExpandAABB(BoundingBox box, Vector3 velocity);
Vector3 COLLISION_AABBCenter(BoundingBox box);
Vector3 COLLISION_AABBExtents(BoundingBox box);

/* ── Penetration (Phase 5) ─────────────────────────────────────── */

/* Returns penetration depth. Normal points from A to B. */
/* Returns <= 0 if no overlap. */
float COLLISION_BoxVsBox(BoundingBox a, BoundingBox b, Vector3* outNormal);
float COLLISION_SphereVsSphere(Vector3 ca, float ra, Vector3 cb, float rb,
    Vector3* outNormal);
float COLLISION_BoxVsSphere(BoundingBox box, Vector3 center, float radius,
    Vector3* outNormal);

/* Phase 5: Capsule tests */
/* float COLLISION_CapsuleVsBox(...); */
/* float COLLISION_CapsuleVsSphere(...); */
/* float COLLISION_CapsuleVsCapsule(...); */
/* float COLLISION_CapsuleVsPlane(...); */

/* ── Distance Queries ──────────────────────────────────────────── */

/* Closest point on line segment AB to point P. */
Vector3 COLLISION_ClosestPointOnSegment(Vector3 a, Vector3 b, Vector3 p);

/* Squared distance from point to AABB. */
float COLLISION_PointToAABBDistSq(Vector3 point, BoundingBox box);