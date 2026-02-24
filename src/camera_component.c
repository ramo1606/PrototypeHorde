#include "camera_component.h"
#include "actor.h"
#include "game.h"
#include "renderer.h"
#include <assert.h>

void CAMERA_COMPONENT_Init(CameraComponent* cc, Actor* owner)
{
    assert(cc != NULL);
    assert(owner != NULL);

    SCENE_COMPONENT_Init(&cc->scene, owner, COMPONENT_CAMERA, 250);
    SCENE_COMPONENT_AttachChild(&owner->root, &cc->scene);

    cc->camera = (Camera3D){
        .position = (Vector3){ 0.0f, 5.0f, -10.0f },
        .target = (Vector3){ 0.0f, 0.0f, 0.0f },
        .up = (Vector3){ 0.0f, 1.0f, 0.0f },
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
}

void CAMERA_COMPONENT_Apply(CameraComponent* cc)
{
    assert(cc != NULL);

    Actor* owner = CAMERA_COMPONENT_GetOwner(cc);
    Game* game = owner->game;
    RENDERER_SetCamera(&game->renderer, cc->camera);
}

Actor* CAMERA_COMPONENT_GetOwner(CameraComponent* cc)
{
    assert(cc != NULL);
    return cc->scene.base.owner;
}

Camera3D CAMERA_COMPONENT_GetCamera(CameraComponent* cc)
{
    assert(cc != NULL);
    return cc->camera;
}