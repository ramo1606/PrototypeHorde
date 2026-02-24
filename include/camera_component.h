#pragma once

#include "scene_component.h"
#include "raylib.h"

typedef struct Actor Actor;

typedef struct
{
    SceneComponent scene;
    Camera3D  camera;
} CameraComponent;

void CAMERA_COMPONENT_Init(CameraComponent* cc, Actor* owner);
void CAMERA_COMPONENT_Apply(CameraComponent* cc);
Actor* CAMERA_COMPONENT_GetOwner(CameraComponent* cc);
Camera3D CAMERA_COMPONENT_GetCamera(CameraComponent* cc);