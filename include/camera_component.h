#pragma once

#include "component.h"
#include "raylib.h"

typedef struct Actor Actor;

typedef struct
{
    Component base;
    Camera3D  cam;
} CameraComponent;

void CAMERA_COMPONENT_Init(CameraComponent* cc, Actor* owner);
void CAMERA_COMPONENT_Apply(CameraComponent* cc);