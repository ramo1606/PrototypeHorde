/*******************************************************************************************
*
*   collision.c — Collision Math Utilities Implementation
*
********************************************************************************************/
#include "collision.h"

/*------------------------------------------------------------------------------------
 * COLLISION_TransformAABB
 *
 *   Transforms a local-space AABB into world space using a transformation matrix.
 *
 *   Algorithm:
 *   A naive approach (just transforming min and max) fails under rotation because
 *   the min/max corners may swap axes. Instead, we transform ALL 8 corners of the
 *   box and compute new min/max from the transformed points.
 *
 *   The 8 corners are every combination of (min.x|max.x, min.y|max.y, min.z|max.z).
 *
 *   This produces a correct axis-aligned bounding box in world space, though it
 *   may be slightly larger than the rotated object (conservative approximation).
 *
 *   Time complexity: O(8) = O(1) — always 8 corners regardless of mesh complexity.
 *
 *   Reference: "Real-Time Collision Detection" by Christer Ericson, Section 4.2.6
 *------------------------------------------------------------------------------------*/
BoundingBox COLLISION_TransformAABB(BoundingBox local, Matrix transform)
{
    /* Enumerate all 8 corners of the local AABB */
    Vector3 corners[8] = {
        { local.min.x, local.min.y, local.min.z },
        { local.min.x, local.min.y, local.max.z },
        { local.min.x, local.max.y, local.min.z },
        { local.min.x, local.max.y, local.max.z },
        { local.max.x, local.min.y, local.min.z },
        { local.max.x, local.min.y, local.max.z },
        { local.max.x, local.max.y, local.min.z },
        { local.max.x, local.max.y, local.max.z },
    };

    /* Transform first corner to initialize min/max */
    Vector3 first = Vector3Transform(corners[0], transform);
    BoundingBox result = { .min = first, .max = first };

    /* Expand min/max with remaining corners */
    for (int i = 1; i < 8; i++)
    {
        Vector3 p = Vector3Transform(corners[i], transform);
        result.min.x = fminf(result.min.x, p.x);
        result.min.y = fminf(result.min.y, p.y);
        result.min.z = fminf(result.min.z, p.z);
        result.max.x = fmaxf(result.max.x, p.x);
        result.max.y = fmaxf(result.max.y, p.y);
        result.max.z = fmaxf(result.max.z, p.z);
    }

    return result;
}

/*------------------------------------------------------------------------------------
 * COLLISION_MergeAABB
 *
 *   Creates the smallest AABB that fully contains both input AABBs.
 *   Simply takes the component-wise min of the mins and max of the maxes.
 *
 *   Used for combining bounding boxes (e.g., computing scene bounds).
 *------------------------------------------------------------------------------------*/
BoundingBox COLLISION_MergeAABB(BoundingBox a, BoundingBox b)
{
    BoundingBox result;
    result.min.x = fminf(a.min.x, b.min.x);
    result.min.y = fminf(a.min.y, b.min.y);
    result.min.z = fminf(a.min.z, b.min.z);
    result.max.x = fmaxf(a.max.x, b.max.x);
    result.max.y = fmaxf(a.max.y, b.max.y);
    result.max.z = fmaxf(a.max.z, b.max.z);
	return result;
}

/*------------------------------------------------------------------------------------
 * COLLISION_ExpandAABB
 *
 *   Expands an AABB along a velocity vector. This produces a "swept" AABB that
 *   covers the box's path over one timestep, useful for broad-phase swept collision.
 *
 *   For each axis: if velocity is positive, extend max; if negative, extend min.
 *   This is equivalent to the Minkowski sum of the AABB and the velocity segment.
 *------------------------------------------------------------------------------------*/
BoundingBox COLLISION_ExpandAABB(BoundingBox box, Vector3 velocity)
{
    BoundingBox result = box;
    if (velocity.x > 0) result.max.x += velocity.x;
    else result.min.x += velocity.x;
    if (velocity.y > 0) result.max.y += velocity.y;
    else result.min.y += velocity.y;
    if (velocity.z > 0) result.max.z += velocity.z;
    else result.min.z += velocity.z;
	return result;
}

/* Center of an AABB = midpoint of min and max. */
Vector3 COLLISION_AABBCenter(BoundingBox box)
{
    Vector3 center;
    center.x = (box.min.x + box.max.x) * 0.5f;
    center.y = (box.min.y + box.max.y) * 0.5f;
    center.z = (box.min.z + box.max.z) * 0.5f;
	return center;
}

/* Half-extents = half the size of the AABB along each axis. */
Vector3 COLLISION_AABBExtents(BoundingBox box)
{
    Vector3 extents;
    extents.x = (box.max.x - box.min.x) * 0.5f;
    extents.y = (box.max.y - box.min.y) * 0.5f;
    extents.z = (box.max.z - box.min.z) * 0.5f;
	return extents;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Penetration Tests — Stubs (Phase 5)
 *
 *  These will implement SAT (Separating Axis Theorem) for AABB-vs-AABB,
 *  sphere distance tests for sphere pairs, and closest-point tests for
 *  box-vs-sphere. Currently return 0 (no overlap) as placeholders.
 * ═══════════════════════════════════════════════════════════════════════════ */

 /* Stub: AABB vs AABB penetration test. Will use SAT on 3 axes. */
float COLLISION_BoxVsBox(BoundingBox a, BoundingBox b, Vector3* outNormal)
{
	return 0.0f;
}

/* Stub: Sphere vs Sphere penetration test. Will use center distance vs radii sum. */
float COLLISION_SphereVsSphere(Vector3 ca, float ra, Vector3 cb, float rb, Vector3* outNormal)
{
    return 0.0f;
}

/* Stub: AABB vs Sphere penetration test. Will use closest point on box to sphere center. */
float COLLISION_BoxVsSphere(BoundingBox box, Vector3 center, float radius, Vector3* outNormal)
{
    return 0.0f;
}


/* ═══════════════════════════════════════════════════════════════════════════
 *  Distance Queries
 * ═══════════════════════════════════════════════════════════════════════════ */

 /*------------------------------------------------------------------------------------
  * COLLISION_ClosestPointOnSegment
  *
  *   Finds the closest point on line segment AB to a given point P.
  *
  *   Algorithm:
  *   1. Project P onto the infinite line through A and B:
  *      t = dot(P - A, B - A) / dot(B - A, B - A)
  *   2. Clamp t to [0, 1] to stay within the segment.
  *   3. The closest point is A + t × (B - A).
  *
  *   This is the fundamental building block for capsule collision tests,
  *   since a capsule is defined by a line segment + radius.
  *
  *   Reference: "Real-Time Collision Detection" by Christer Ericson, Section 5.1.2
  *------------------------------------------------------------------------------------*/
Vector3 COLLISION_ClosestPointOnSegment(Vector3 a, Vector3 b, Vector3 p)
{
    Vector3 ab = Vector3Subtract(b, a);
    float t = Vector3DotProduct(Vector3Subtract(p, a), ab) / Vector3DotProduct(ab, ab);
    t = fmaxf(0.0f, fminf(1.0f, t));    /* Clamp to segment */
	return Vector3Add(a, Vector3Scale(ab, t));
}

/*------------------------------------------------------------------------------------
 * COLLISION_PointToAABBDistSq
 *
 *   Computes the squared distance from a point to the nearest surface of an AABB.
 *   Returns 0 if the point is inside the box.
 *
 *   Algorithm:
 *   For each axis, compute the distance from the point to the nearest face:
 *       d = max(box.min - point, 0, point - box.max)
 *   Then return dx² + dy² + dz² (squared Euclidean distance).
 *
 *   Using squared distance avoids the sqrt — useful for comparisons where
 *   you can compare squared distances instead.
 *
 *   Reference: "Real-Time Collision Detection" by Christer Ericson, Section 5.1.3
 *------------------------------------------------------------------------------------*/
float COLLISION_PointToAABBDistSq(Vector3 point, BoundingBox box)
{
    float dx = fmaxf(box.min.x - point.x, fmaxf(0.0f, point.x - box.max.x));
    float dy = fmaxf(box.min.y - point.y, fmaxf(0.0f, point.y - box.max.y));
    float dz = fmaxf(box.min.z - point.z, fmaxf(0.0f, point.z - box.max.z));
	return dx * dx + dy * dy + dz * dz;
}
