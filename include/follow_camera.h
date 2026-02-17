#pragma once

#include "camera_component.h"
#include "raymath.h"

typedef struct Actor Actor;

typedef struct
{
    CameraComponent base;

    /* Spring state */
    Vector3 actualPos;
    Vector3 velocity;

    /* Tuning */
    float horzDist;        /* Distance behind the owner */
    float vertDist;        /* Distance above the owner */
    float targetDist;      /* Look-at point in front of owner */
    float springConstant;  /* Higher = stiffer follow */
} FollowCameraComponent;

FollowCameraComponent* FOLLOW_CAMERA_Create(Actor* owner);
void FOLLOW_CAMERA_SnapToIdeal(FollowCameraComponent* fc);