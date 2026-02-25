#pragma once

/*
 * collision.h — Low-level collision geometry utilities.
 *
 * This module provides stateless, data-only collision math functions.  It
 * does NOT own any state — all results are returned by value or via output
 * parameters.  PhysWorld (physics_world.h) calls these routines during its
 * broadphase and query passes.
 *
 * Techniques used:
 *   - AABB transform: 8-corner method — transform all 8 corners of the
 *     local AABB by the world matrix and refit a new min/max AABB.  This
 *     is correct under arbitrary rotation and non-uniform scale.
 *   - Penetration depth: signed axis-overlap tests for box-box, sphere-
 *     sphere, and box-sphere pairs.
 *   - Distance queries: closest point on segment, squared point-to-AABB
 *     distance for broad rejection.
 *
 * Architecture position:
 *   Stateless math layer — used by PhysWorld, BoxComponent, MeshComponent.
 */

#include "raylib.h"
#include "raymath.h"

/* ── AABB Utilities ─────────────────────────────────────────────── */

BoundingBox COLLISION_TransformAABB(BoundingBox local, Matrix transform);  // Transform a local-space AABB to world space using the 8-corner method
BoundingBox COLLISION_MergeAABB(BoundingBox a, BoundingBox b);             // Return the smallest AABB that encloses both a and b
BoundingBox COLLISION_ExpandAABB(BoundingBox box, Vector3 velocity);       // Expand an AABB along each axis in the direction of velocity (for swept tests)
Vector3     COLLISION_AABBCenter(BoundingBox box);                         // Return the centre point of an AABB
Vector3     COLLISION_AABBExtents(BoundingBox box);                        // Return the half-extents (half width/height/depth) of an AABB

/* ── Penetration (Phase 5) ─────────────────────────────────────── */

/* Returns penetration depth. Normal points from A to B. */
/* Returns <= 0 if no overlap. */
float COLLISION_BoxVsBox(BoundingBox a, BoundingBox b, Vector3* outNormal);                              // Compute penetration depth between two AABBs; outNormal points from a to b
float COLLISION_SphereVsSphere(Vector3 ca, float ra, Vector3 cb, float rb, Vector3* outNormal);          // Compute penetration depth between two spheres; outNormal points from a to b
float COLLISION_BoxVsSphere(BoundingBox box, Vector3 center, float radius, Vector3* outNormal);          // Compute penetration depth between an AABB and a sphere; outNormal points outward from box

/* Phase 5: Capsule tests */
/* float COLLISION_CapsuleVsBox(...); */
/* float COLLISION_CapsuleVsSphere(...); */
/* float COLLISION_CapsuleVsCapsule(...); */
/* float COLLISION_CapsuleVsPlane(...); */

/* ── Distance Queries ──────────────────────────────────────────── */

/* Closest point on line segment AB to point P. */
Vector3 COLLISION_ClosestPointOnSegment(Vector3 a, Vector3 b, Vector3 p);  // Return the point on segment AB closest to P (clamped to [0,1] parameter range)

/* Squared distance from point to AABB. */
float COLLISION_PointToAABBDistSq(Vector3 point, BoundingBox box); // Return the squared distance from point to the surface of box (0 if inside)