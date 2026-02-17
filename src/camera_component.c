#include "camera_component.h"
#include "actor.h"
#include "game.h"
#include "renderer.h"
#include <assert.h>

void CAMERA_COMPONENT_Init(CameraComponent* cc, Actor* owner)
{
    assert(cc != NULL);
    assert(owner != NULL);

    COMPONENT_Init(&cc->base, owner, COMPONENT_CAMERA, 250);

    cc->cam = (Camera3D){
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

    Game* game = cc->base.owner->game;
    RENDERER_SetCamera(&game->renderer, cc->cam);
}