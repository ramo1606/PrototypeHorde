#pragma once

#include "camera_component.h"
#include "raymath.h"

typedef struct Actor Actor;

/*
 * CameraTPS — Third-Person Spring camera.
 *
 * Follows the owner actor with critically damped spring physics.
 * Computes position behind + above the owner, looks at a point
 * ahead of the owner.
 *
 * Embedding chain:
 *   CameraTPS
 *   └── CameraComponent base
 *       └── SpatialComponent spatial   (offset 0)
 *           └── Component base         (offset 0)
 */

typedef struct
{
    CameraComponent cameraComponent;

    /* Spring state */
    Vector3 actualPos;
    Vector3 velocity;

    /* Tuning */
    float horzDist;        /* Distance behind the owner */
    float vertDist;        /* Distance above the owner */
    float targetDist;      /* Look-at point in front of owner */
    float springConstant;  /* Higher = stiffer follow */
} CameraTPS;

CameraTPS* CAMERA_TPS_Create(Actor* owner);
void CAMERA_TPS_SnapToIdeal(CameraTPS* ctps);