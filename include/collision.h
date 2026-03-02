/*******************************************************************************************
*
*   collision.h — Collision Math Utilities
*
*   Pure math functions for geometric collision tests. This module has NO dependencies
*   on the game systems — it only uses Raylib's math types (Vector3, Matrix,
*   BoundingBox). This makes it easy to test and reuse.
*
*   Three categories of functions:
*
*   1. AABB Utilities:
*       Transform, merge, expand, center, and extents for axis-aligned bounding boxes.
*       Used by BoxComponent and Renderer for frustum culling.
*
*   2. Penetration Tests:
*       Return overlap depth and collision normal between shape pairs.
*       Used by PhysWorld for collision response (Phase 5).
*       Currently stubbed (return 0.0f) — will be implemented when needed.
*
*   3. Distance Queries:
*       Closest point on segment, point-to-AABB distance.
*       Used for spatial queries and capsule collision (Phase 5).
*
*   Naming Convention:
*       API:     COLLISION_*
*
********************************************************************************************/
#pragma once

#include "raylib.h"
#include "raymath.h"

/* ── AABB Utilities ──────────────────────────────────────────────────────── */

/* Transform a local AABB by a matrix, producing a correct world-space AABB. */
BoundingBox COLLISION_TransformAABB(BoundingBox local, Matrix transform);

/* Compute the smallest AABB that contains both input AABBs. */
BoundingBox COLLISION_MergeAABB(BoundingBox a, BoundingBox b);

/* Expand an AABB along a velocity vector (for swept collision). */
BoundingBox COLLISION_ExpandAABB(BoundingBox box, Vector3 velocity);

/* Get the center point of an AABB. */
Vector3 COLLISION_AABBCenter(BoundingBox box);

/* Get the half-extents (half-size) of an AABB. */
Vector3 COLLISION_AABBExtents(BoundingBox box);

/* ── Penetration Tests (Phase 5) ─────────────────────────────────────────── */
/* Returns penetration depth. Normal points from A to B.                      */
/* Returns <= 0 if no overlap.                                                */
float COLLISION_BoxVsBox(BoundingBox a, BoundingBox b, Vector3* outNormal);
float COLLISION_SphereVsSphere(Vector3 ca, float ra, Vector3 cb, float rb,
    Vector3* outNormal);
float COLLISION_BoxVsSphere(BoundingBox box, Vector3 center, float radius,
    Vector3* outNormal);

/* Phase 5: Capsule tests (not yet implemented) */
/* float COLLISION_CapsuleVsBox(...); */
/* float COLLISION_CapsuleVsSphere(...); */
/* float COLLISION_CapsuleVsCapsule(...); */
/* float COLLISION_CapsuleVsPlane(...); */

/* ── Distance Queries ────────────────────────────────────────────────────── */

/* Closest point on line segment AB to point P. */
Vector3 COLLISION_ClosestPointOnSegment(Vector3 a, Vector3 b, Vector3 p);

/* Squared distance from a point to the nearest surface of an AABB. */
float COLLISION_PointToAABBDistSq(Vector3 point, BoundingBox box);