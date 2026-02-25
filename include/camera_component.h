#pragma once

/*
 * camera_component.h — Scene-attached camera component.
 *
 * CameraComponent extends SceneComponent so it can live in the scene graph
 * as a child of an actor's root.  It wraps Raylib's Camera3D and exposes
 * CAMERA_COMPONENT_Apply() to push the current camera state into the
 * Renderer every frame.
 *
 * CameraTPS (camera_tps.h) derives from CameraComponent by embedding it as
 * its first field and adds spring-damper follow behaviour on top.
 *
 * Architecture position:
 *   Actor.root → CameraComponent.scene (child SceneComponent)
 *   CameraComponent → Renderer.camera (via Apply)
 */

#include "scene_component.h"
#include "raylib.h"

typedef struct CameraComponent CameraComponent;
typedef struct Actor Actor;

/* ── CameraComponent Struct ─────────────────────────────────────── */

struct CameraComponent
{
    SceneComponent scene;  /* Embedded SceneComponent — provides hierarchy attachment and world transform */
    Camera3D       camera; /* Raylib 3D camera (position, target, up, fovy, projection) */
};

/* ── Public API ─────────────────────────────────────────────────── */

void     CAMERA_COMPONENT_Init(CameraComponent* cc, Actor* owner);   // Initialise the component, attach it to the actor's scene graph, and set default camera values
void     CAMERA_COMPONENT_Apply(CameraComponent* cc);                // Push the camera into the renderer so it is used for the next DrawFrame call
Actor*   CAMERA_COMPONENT_GetOwner(CameraComponent* cc);             // Return the actor that owns this component
Camera3D CAMERA_COMPONENT_GetCamera(CameraComponent* cc);            // Return a copy of the current Camera3D state