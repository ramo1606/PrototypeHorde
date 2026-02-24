#pragma once

#include "scene_component.h"
#include "raylib.h"

typedef struct CameraComponent CameraComponent;
typedef struct Actor Actor;

struct CameraComponent
{
    SceneComponent scene;
    Camera3D  camera;
};

void CAMERA_COMPONENT_Init(CameraComponent* cc, Actor* owner);
void CAMERA_COMPONENT_Apply(CameraComponent* cc);
Actor* CAMERA_COMPONENT_GetOwner(CameraComponent* cc);
Camera3D CAMERA_COMPONENT_GetCamera(CameraComponent* cc);