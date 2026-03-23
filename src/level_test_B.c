/*******************************************************************************************
*
*   level_test_b.c — Test Level B: red background + rotating sphere
*
*   Press SPACE to transition back to Level A (uses wipe effect).
*
********************************************************************************************/

#include "level_test_b.h"
#include "level_test_a.h"
#include "game.h"
#include "memory.h"
#include "config.h"
#include "level_manager.h"
#include "raylib.h"
#include <math.h>

typedef struct 
{
    float    angle;
    float    anglePrev;
    Camera3D camera;
} TestBData;

static TestBData* data = NULL;

static void Init(Game* game)
{
    data = ARENA_ALLOC(&game->level, TestBData);
    data->angle = 0.0f;
    data->anglePrev = 0.0f;
    data->camera = (Camera3D){
        .position = (Vector3){ 5.0f, 3.0f, 5.0f },
        .target = (Vector3){ 0.0f, 0.0f, 0.0f },
        .up = (Vector3){ 0.0f, 1.0f, 0.0f },
        .fovy = 60.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
}

static void Shutdown(Game* game) { (void)game; data = NULL; }

static void ProcessInput(Game* game)
{
    /* Use a wipe transition to show a different effect */
    if (IsKeyPressed(KEY_SPACE))
        LEVEL_MGR_TransitionTo(&game->levelMgr, &LEVEL_TEST_A,
            TRANSITION_WipeLeft, TRANSITION_WipeRight, 0.5f);
}

static void Update(Game* game, float dt)
{
    (void)game;
    data->anglePrev = data->angle;
    data->angle += 120.0f * dt;
    if (data->angle > 360.0f) 
    {
        data->angle -= 360.0f;
        data->anglePrev -= 360.0f;
    }
}

static void Render3D(Game* game, float alpha)
{
    (void)game;
    float a = data->anglePrev + (data->angle - data->anglePrev) * alpha;

    DrawSphere((Vector3) { 0, 1, 0 }, 0.8f, RED);
    DrawSphereWires((Vector3) { 0, 1, 0 }, 0.8f, 8, 8, MAROON);
    DrawGrid(10, 1.0f);

    float rad = a * DEG2RAD;
    DrawCube((Vector3) { cosf(rad) * 3, 0.3f, sinf(rad) * 3 }, 0.4f, 0.4f, 0.4f, ORANGE);
}

static void RenderHUD(Game* game, float alpha)
{
    (void)alpha;

    DrawText("LEVEL B (red)", 10, SCREEN_HEIGHT - 60, 20, RAYWHITE);
    DrawText("Press SPACE -> Level A (wipe transition)", 10, SCREEN_HEIGHT - 35, 16, LIGHTGRAY);
}

Level LEVEL_TEST_B = 
{
    .name = "Test B",
    .Init = Init,
    .Shutdown = Shutdown,
    .ProcessInput = ProcessInput,
    .Update = Update,
    .Render3D = Render3D,
    .RenderHUD = RenderHUD,
};
