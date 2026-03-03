/*******************************************************************************************
*
*   camera_component.c — Camera Component Implementation
*
********************************************************************************************/
#include "camera_component.h"
#include "actor.h"
#include "game.h"
#include "renderer.h"
#include <assert.h>

/*------------------------------------------------------------------------------------
 * CAMERA_COMPONENT_Init
 *
 *   Initializes the camera's SceneComponent base, attaches as a child of the
 *   actor's root, and sets default camera parameters.
 *
 *   Update order 250 places camera updates after movement (10) and mesh (200)
 *   but before colliders (300). This ensures the camera sees the current frame's
 *   actor position when it computes its view.
 *
 *   Default camera: perspective, 45° FOV, positioned behind and above origin.
 *------------------------------------------------------------------------------------*/
void CAMERA_COMPONENT_Init(CameraComponent* cc, Actor* owner)
{
    assert(cc != NULL);
    assert(owner != NULL);

    SCENE_COMPONENT_Init(&cc->scene, owner, COMPONENT_TYPE_CAMERA, 250);
    SCENE_COMPONENT_AttachChild(&owner->root, &cc->scene);

    cc->camera = (Camera3D){
        .position = (Vector3){ 0.0f, 5.0f, -10.0f },
        .target = (Vector3){ 0.0f, 0.0f, 0.0f },
        .up = (Vector3){ 0.0f, 1.0f, 0.0f },
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
}

/*------------------------------------------------------------------------------------
 * CAMERA_COMPONENT_Apply
 *
 *   Pushes this camera's Camera3D struct to the Renderer. The Renderer uses the
 *   active camera for view matrix computation, frustum extraction, and 3D rendering.
 *   Only one camera is active at a time — calling Apply overrides the previous one.
 *------------------------------------------------------------------------------------*/
void CAMERA_COMPONENT_Apply(CameraComponent* cc)
{
    assert(cc != NULL);

    Actor* owner = CAMERA_COMPONENT_GetOwner(cc);
    Game* game = owner->game;
    RENDERER_SetCamera(&game->renderer, cc->camera);
}

/* Get the Actor owner through the SceneComponent → Component chain. */
Actor* CAMERA_COMPONENT_GetOwner(CameraComponent* cc)
{
    assert(cc != NULL);
    return cc->scene.base.owner;
}

/* Return a copy of the camera struct. */
Camera3D CAMERA_COMPONENT_GetCamera(CameraComponent* cc)
{
    assert(cc != NULL);
    return cc->camera;
}