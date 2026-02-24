#pragma once

#include "camera_component.h"
#include "raymath.h"

typedef struct CameraTPS CameraTPS;
typedef struct Actor Actor;

struct CameraTPS
{
    CameraComponent base;

    Vector3 actualPos;
    Vector3 velocity;

    float horzDist;
    float vertDist;
    float targetDist;
    float springConstant;
};

CameraTPS* CAMERA_TPS_Create(Actor* owner);
void CAMERA_TPS_SnapToIdeal(CameraTPS* ctps);

void CAMERA_TPS_SetDistances(CameraTPS* ctps, float horz, float vert, float target);
void CAMERA_TPS_SetSpring(CameraTPS* ctps, float springConstant);