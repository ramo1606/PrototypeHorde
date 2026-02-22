#include "collision.h"

BoundingBox COLLISION_TransformAABB(BoundingBox local, Matrix transform)
{
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