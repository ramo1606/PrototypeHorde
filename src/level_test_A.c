/*******************************************************************************************
*
*   level_test_a.c — Test Level A: demonstrates the Renderer pipeline
*
*   This level creates a cube model programmatically, registers it with
*   the renderer, and updates its transform each tick. The renderer handles
*   interpolation, clear, BeginMode3D, drawing, and EndMode3D.
*
*   Press SPACE to transition to Level B.
*
********************************************************************************************/

#include "level_test_a.h"
#include "level_test_b.h"
#include "game.h"
#include "memory.h"
#include "config.h"
#include "level_manager.h"
#include "renderer.h"
#include "raylib.h"
#include <math.h>

typedef struct 
{
    float        rotation;
    float        rotationPrev;
    Model        cubeModel;          /* Generated cube mesh → model */
    Model        markerModel;        /* Small sphere for the orbiting marker */
    RenderHandle cubeHandle;         /* Handle in the renderer */
    RenderHandle markerHandle;
} TestAData;

static TestAData* data = NULL;

static void Init(Game* game)
{
    data = ARENA_ALLOC(&game->level, TestAData);
    data->rotation = 0.0f;
    data->rotationPrev = 0.0f;

    /* Create models from generated meshes (no external files needed) */
    data->cubeModel = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    data->markerModel = LoadModelFromMesh(GenMeshSphere(0.2f, 8, 8));

    /* Register with the renderer — this is the pattern all gameplay
     * entities will follow: create model, register, get handle. */
    data->cubeHandle = RENDERER_Register(&game->renderer, data->cubeModel, 0);
    data->markerHandle = RENDERER_Register(&game->renderer, data->markerModel, 0);

    /* Set initial transforms */
    RENDERER_SetTransform(&game->renderer, data->cubeHandle,
        MatrixTranslate(0.0f, 0.5f, 0.0f));

    /* Configure the renderer's camera for this level */
    RENDERER_SetCamera(&game->renderer, (Camera3D) {
        .position = (Vector3){ 4.0f, 4.0f, 4.0f },
            .target = (Vector3){ 0.0f, 0.5f, 0.0f },
            .up = (Vector3){ 0.0f, 1.0f, 0.0f },
            .fovy = 60.0f,
            .projection = CAMERA_PERSPECTIVE,
    });
    RENDERER_SetClearColor(&game->renderer, (Color) { 25, 40, 80, 255 });
}

static void Shutdown(Game* game)
{
    /* Unregister from renderer BEFORE models are unloaded */
    if (data) {
        RENDERER_Unregister(&game->renderer, data->cubeHandle);
        RENDERER_Unregister(&game->renderer, data->markerHandle);

        /* Unload the generated models (free GPU resources) */
        UnloadModel(data->cubeModel);
        UnloadModel(data->markerModel);
    }
    data = NULL;
}

static void ProcessInput(Game* game)
{
    if (IsKeyPressed(KEY_SPACE))
        LEVEL_MGR_SwitchTo(&game->levelMgr, &LEVEL_TEST_B);
}

static void Update(Game* game, float dt)
{
    (void)game;

    data->rotationPrev = data->rotation;
    data->rotation += 90.0f * dt;
    if (data->rotation > 360.0f) {
        data->rotation -= 360.0f;
        data->rotationPrev -= 360.0f;
    }

    /*
     * Update transforms in the renderer.
     * Note: we set transformCurr here. The renderer already copied
     * curr→prev in RENDERER_PreUpdate (called by game loop before us).
     * So prev has LAST tick's value, and curr gets THIS tick's value.
     */
    RENDERER_SetTransform(&game->renderer, data->cubeHandle,
        MatrixMultiply(
            MatrixRotateY(data->rotation * DEG2RAD),
            MatrixTranslate(0.0f, 0.5f, 0.0f)
        ));

    float rad = data->rotation * DEG2RAD;
    RENDERER_SetTransform(&game->renderer, data->markerHandle,
        MatrixTranslate(cosf(rad) * 2.0f, 0.5f, sinf(rad) * 2.0f));

    /*
     * Free camera for testing — WASD + mouse.
     * This is a temporary debug aid, NOT the final camera system (Task 1.6).
     * UpdateCamera reads mouse/keyboard input and moves the camera.
     */
    Camera3D cam = RENDERER_GetCamera(&game->renderer);
    UpdateCamera(&cam, CAMERA_FREE);
    RENDERER_SetCamera(&game->renderer, cam);
}

static void Render3D(Game* game, float alpha)
{
    (void)game;
    (void)alpha;

    /* Grid for reference (will be replaced by arena floor) */
    DrawGrid(20, 1.0f);
}

static void RenderHUD(Game* game, float alpha)
{
    (void)alpha;

    DrawText("LEVEL A — Renderer pipeline", 10, SCREEN_HEIGHT - 80, 20, RAYWHITE);
    DrawText(TextFormat("Drawn: %d  Culled: %d",
        game->renderer.statsDrawn, game->renderer.statsCulled),
        10, SCREEN_HEIGHT - 55, 16, GREEN);
    DrawText("Press SPACE -> Level B", 10, SCREEN_HEIGHT - 30, 16, LIGHTGRAY);
}

Level LEVEL_TEST_A = 
{
    .name = "Test A (renderer)",
    .Init = Init,
    .Shutdown = Shutdown,
    .ProcessInput = ProcessInput,
    .Update = Update,
    .Render3D = Render3D,
    .RenderHUD = RenderHUD,
};
