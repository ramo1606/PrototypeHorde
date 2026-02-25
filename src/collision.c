#include "collision.h"

BoundingBox COLLISION_TransformAABB(BoundingBox local, Matrix transform)
{
    /*
     * 8-corner AABB transform.
     *
     * A naive approach of transforming only min and max does not work under
     * rotation because the transformed corners may not align with the axes.
     * The correct approach is to transform all 8 corners of the local AABB
     * and refit a new axis-aligned box around the results.
     *
     * This is conservative (may be slightly larger than the tightest fit)
     * but is exact for pure scale+rotation without shear.
     *
     * Time complexity: O(8) — constant regardless of mesh complexity.
     */
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

    Vector3 first = Vector3Transform(corners[0], transform);
    BoundingBox result = { .min = first, .max = first };

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

BoundingBox COLLISION_MergeAABB(BoundingBox a, BoundingBox b)
{
    /*
     * Component-wise min/max to produce the smallest AABB enclosing both
     * inputs.  Useful for combining bounding boxes of sub-meshes.
     */
    BoundingBox result;
    result.min.x = fminf(a.min.x, b.min.x);
    result.min.y = fminf(a.min.y, b.min.y);
    result.min.z = fminf(a.min.z, b.min.z);
    result.max.x = fmaxf(a.max.x, b.max.x);
    result.max.y = fmaxf(a.max.y, b.max.y);
    result.max.z = fmaxf(a.max.z, b.max.z);
	return result;
}

BoundingBox COLLISION_ExpandAABB(BoundingBox box, Vector3 velocity)
{
    /*
     * Expand the AABB in the direction of velocity on each axis.
     * Positive velocity extends max; negative velocity extends min.
     * Used to create a swept AABB for continuous collision detection.
     */
    BoundingBox result = box;
    if (velocity.x > 0) result.max.x += velocity.x;
    else result.min.x += velocity.x;
    if (velocity.y > 0) result.max.y += velocity.y;
    else result.min.y += velocity.y;
    if (velocity.z > 0) result.max.z += velocity.z;
    else result.min.z += velocity.z;
	return result;
}

Vector3 COLLISION_AABBCenter(BoundingBox box)
{
    /*
     * Average of min and max gives the geometric centre of the AABB.
     */
    Vector3 center;
    center.x = (box.min.x + box.max.x) * 0.5f;
    center.y = (box.min.y + box.max.y) * 0.5f;
    center.z = (box.min.z + box.max.z) * 0.5f;
	return center;
}

Vector3 COLLISION_AABBExtents(BoundingBox box)
{
    /*
     * Half the size along each axis.  Used as the radius in SAT-style
     * overlap tests.
     */
    Vector3 extents;
    extents.x = (box.max.x - box.min.x) * 0.5f;
    extents.y = (box.max.y - box.min.y) * 0.5f;
    extents.z = (box.max.z - box.min.z) * 0.5f;
	return extents;
}

float COLLISION_BoxVsBox(BoundingBox a, BoundingBox b, Vector3* outNormal)
{
	return 0.0f;
}

float COLLISION_SphereVsSphere(Vector3 ca, float ra, Vector3 cb, float rb, Vector3* outNormal)
{
    return 0.0f;
}

float COLLISION_BoxVsSphere(BoundingBox box, Vector3 center, float radius, Vector3* outNormal)
{
    return 0.0f;
}

Vector3 COLLISION_ClosestPointOnSegment(Vector3 a, Vector3 b, Vector3 p)
{
    /*
     * Project point P onto line AB, clamp the parameter t to [0,1] to
     * stay on the segment, and return the clamped position.
     *
     *   t = dot(P-A, B-A) / dot(B-A, B-A)
     *   result = A + t * (B-A)
     *
     * Used in capsule vs. primitive distance tests.
     */
    Vector3 ab = Vector3Subtract(b, a);
    float t = Vector3DotProduct(Vector3Subtract(p, a), ab) / Vector3DotProduct(ab, ab);
    t = fmaxf(0.0f, fminf(1.0f, t));
	return Vector3Add(a, Vector3Scale(ab, t));
}

float COLLISION_PointToAABBDistSq(Vector3 point, BoundingBox box)
{
    /*
     * For each axis independently, compute the distance from the point to
     * the nearest face of the box (0 if inside on that axis).  Sum the
     * squared per-axis distances.  Result is 0 if the point is inside the
     * box.  Used as a fast broad-reject before more expensive tests.
     */
    float dx = fmaxf(box.min.x - point.x, fmaxf(0.0f, point.x - box.max.x));
    float dy = fmaxf(box.min.y - point.y, fmaxf(0.0f, point.y - box.max.y));
    float dz = fmaxf(box.min.z - point.z, fmaxf(0.0f, point.z - box.max.z));
	return dx * dx + dy * dy + dz * dz;
}
