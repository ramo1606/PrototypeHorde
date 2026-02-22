#pragma once

#include "raylib.h"
#include "raymath.h"

BoundingBox COLLISION_TransformAABB(BoundingBox local, Matrix transform);