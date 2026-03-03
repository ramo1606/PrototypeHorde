/*******************************************************************************************
*
*   camera_component.h — Camera Component (Base Camera)
*
*   CameraComponent wraps a Raylib Camera3D and attaches it to an actor via
*   SceneComponent inheritance. It serves as the base class for specialized cameras
*   (e.g., CameraTPS for third-person).
*
*   Architecture:
*       Actor.root (SceneComponent)
*           └── CameraComponent.scene (child SceneComponent)
*                   └── Camera3D camera (Raylib camera struct)
*
*   When Apply() is called, the camera's settings are pushed to the Renderer,
*   which uses them for the next frame's view/projection matrices.
*
*   Naming Convention:
*       API:     CAMERA_COMPONENT_*
*
********************************************************************************************/
#pragma once

#include "scene_component.h"
#include "raylib.h"

typedef struct CameraComponent CameraComponent;
typedef struct Actor Actor;

/* ── Camera Component Struct ─────────────────────────────────────────────── */
struct CameraComponent
{
    SceneComponent scene;       /* Inherited scene component (must be first field)  */
    Camera3D  camera;           /* Raylib camera: position, target, up, fov, proj   */
};

/* ── Public API ──────────────────────────────────────────────────────────── */
void CAMERA_COMPONENT_Init(CameraComponent* cc, Actor* owner);
void CAMERA_COMPONENT_Apply(CameraComponent* cc);
Actor* CAMERA_COMPONENT_GetOwner(CameraComponent* cc);
Camera3D CAMERA_COMPONENT_GetCamera(CameraComponent* cc);