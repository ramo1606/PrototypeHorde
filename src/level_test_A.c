/*******************************************************************************************
*
*   level_test_a.c — Test Level A: Renderer + Orbit Camera
*
*   A movable "dummy target" controlled with WASD, moving RELATIVE to
*   the camera direction (Uncharted-style). Mouse orbits the camera.
*
*   Controls:
*     WASD            — move dummy (relative to camera facing)
*     Mouse           — orbit camera around dummy
*     Right click     — toggle AIM mode (over-the-shoulder)
*     SPACE           — transition to Level B
*
********************************************************************************************/

#include "level_test_A.h"
#include "level_test_B.h"
#include "game.h"
#include "memory.h"
#include "config.h"
#include "level_manager.h"
#include "renderer.h"
#include "camera.h"
#include "raylib.h"
#include "raymath.h"
#include <math.h>

/* ── Per-level data ──────────────────────────────────────────────────────── */

typedef struct
{
    /* Orbiting marker (tests frustum culling) */
    float        rotation;
    float        rotationPrev;
    Model        markerModel;
    RenderHandle markerHandle;

    /* Dummy target (simulates player) */
    Vector3      dummyPos;
    float        dummyYaw;           /* Visual facing (radians) */
    float        dummySpeed;
    Model        dummyModel;
    RenderHandle dummyHandle;
    ColliderHandle dummyColl;        /* Capsule collider */

    /* Static reference cube at origin */
    Model        cubeModel;
    RenderHandle cubeHandle;
    ColliderHandle cubeColl;         /* Box collider */

    /* Obstacle boxes (static scenery) */
    Model        obstacleModel;
    RenderHandle obstacleHandles[3];
    ColliderHandle obstacleColl[3];
} TestAData;

static TestAData* data = NULL;

/* ═══════════════════════════════════════════════════════════════════════════
 *  Lifecycle
 * ═══════════════════════════════════════════════════════════════════════════ */

static void Init(Game* game)
{
    data = ARENA_ALLOC(&game->level, TestAData);

    /* Orbiting marker */
    data->rotation = 0.0f;
    data->rotationPrev = 0.0f;
    data->markerModel = LoadModelFromMesh(GenMeshSphere(0.2f, 8, 8));
    data->markerHandle = RENDERER_Register(&game->renderer, data->markerModel, 0);

    /* Static reference cube at origin */
    data->cubeModel = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    data->cubeHandle = RENDERER_Register(&game->renderer, data->cubeModel, 0);
    RENDERER_SetTransform(&game->renderer, data->cubeHandle,
        MatrixTranslate(0.0f, 0.5f, 0.0f));

    RENDERER_SetBlobShadow(&game->renderer, data->cubeHandle, true, 0.7f);
    data->cubeColl = PHYS_WORLD_AddBox(&game->physWorld,
        (Vector3){ 0.0f, 0.5f, 0.0f },
        (Vector3){ 0.5f, 0.5f, 0.5f },
        COLLISION_LAYER_SCENERY, COLLISION_MASK_ALL, -1,
        false, false);

    /* Dummy target */
    data->dummyPos = (Vector3){ 0.0f, 0.0f, 3.0f };
    data->dummyYaw = 0.0f;
    data->dummySpeed = 5.0f;
    data->dummyModel = LoadModelFromMesh(GenMeshCube(0.1f, 0.1f, 0.1f));
    data->dummyHandle = RENDERER_Register(&game->renderer, data->dummyModel, 0);
    RENDERER_SetTransform(&game->renderer, data->dummyHandle,
        MatrixTranslate(data->dummyPos.x, 0.5f, data->dummyPos.z));
    RENDERER_SetBlobShadow(&game->renderer, data->dummyHandle, true, 0.5f);
    data->dummyColl = PHYS_WORLD_AddCapsule(&game->physWorld,
        (Vector3){ data->dummyPos.x, 0.5f, data->dummyPos.z },
        0.3f, 0.2f,
        COLLISION_LAYER_PLAYER, COLLISION_LAYER_SCENERY | COLLISION_LAYER_ENEMY, 0,
        true, false);

    /* Obstacle boxes — static scenery for collision testing */
    data->obstacleModel = LoadModelFromMesh(GenMeshCube(2.0f, 1.0f, 1.0f));
 
    Vector3 obstaclePositions[3] = {
        {  4.0f, 0.5f,  0.0f },
        { -3.0f, 0.5f,  2.0f },
        {  1.0f, 0.5f, -4.0f },
    };
 
    for (int i = 0; i < 3; i++)
    {
        data->obstacleHandles[i] = RENDERER_Register(&game->renderer, data->obstacleModel, 0);
        RENDERER_SetTransform(&game->renderer, data->obstacleHandles[i],
            MatrixTranslate(obstaclePositions[i].x, obstaclePositions[i].y, obstaclePositions[i].z));
        RENDERER_SetBlobShadow(&game->renderer, data->obstacleHandles[i], true, 1.2f);
 
        data->obstacleColl[i] = PHYS_WORLD_AddBox(&game->physWorld,
            obstaclePositions[i],
            (Vector3){ 1.0f, 0.5f, 0.5f },
            COLLISION_LAYER_SCENERY, COLLISION_MASK_ALL, -1,
            false, false);
    }

    /* Camera starts looking at the dummy */
    GAME_CAMERA_SetTarget(&game->camera, data->dummyPos);

    /* Lock and hide cursor for mouse orbit */
    DisableCursor();

    RENDERER_SetClearColor(&game->renderer, (Color) { 25, 40, 80, 255 });
}

static void Shutdown(Game* game)
{
    EnableCursor();

    if (data)
    {
        /* Remove colliders */
        PHYS_WORLD_Remove(&game->physWorld, data->dummyColl);
        PHYS_WORLD_Remove(&game->physWorld, data->cubeColl);
        for (int i = 0; i < 3; i++)
            PHYS_WORLD_Remove(&game->physWorld, data->obstacleColl[i]);

        /* Unregister renderables */
        RENDERER_Unregister(&game->renderer, data->markerHandle);
        RENDERER_Unregister(&game->renderer, data->cubeHandle);
        RENDERER_Unregister(&game->renderer, data->dummyHandle);
        for (int i = 0; i < 3; i++)
            RENDERER_Unregister(&game->renderer, data->obstacleHandles[i]);

        UnloadModel(data->markerModel);
        UnloadModel(data->cubeModel);
        UnloadModel(data->dummyModel);
        UnloadModel(data->obstacleModel);
    }
    data = NULL;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Input
 * ═══════════════════════════════════════════════════════════════════════════ */

static void ProcessInput(Game* game)
{
    if (IsKeyPressed(KEY_SPACE))
    {
        EnableCursor();
        LEVEL_MGR_SwitchTo(&game->levelMgr, &LEVEL_TEST_B);
    }

    /* Toggle AIM mode with right mouse button */
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    {
        CameraMode current = game->camera.mode;
        GAME_CAMERA_SetMode(&game->camera,
            current == CAMERA_MODE_FOLLOW ? CAMERA_MODE_AIM : CAMERA_MODE_FOLLOW);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Update (fixed timestep)
 * ═══════════════════════════════════════════════════════════════════════════ */

static void Update(Game* game, float dt)
{
    /* ── Orbiting marker ──────────────────────────────────────────────── */
    data->rotationPrev = data->rotation;
    data->rotation += 90.0f * dt;
    if (data->rotation > 360.0f) {
        data->rotation -= 360.0f;
        data->rotationPrev -= 360.0f;
    }

    float rad = data->rotation * DEG2RAD;
    RENDERER_SetTransform(&game->renderer, data->markerHandle,
        MatrixTranslate(cosf(rad) * 2.0f, 0.5f, sinf(rad) * 2.0f));

    /* ── Mouse orbit ──────────────────────────────────────────────────── */
    Vector2 mouseDelta = GetMouseDelta();
    GAME_CAMERA_RotateByMouse(&game->camera, mouseDelta.x, mouseDelta.y);

    /* ── Dummy target movement (camera-relative) ──────────────────────── */
    float inputForward = 0.0f;
    float inputRight = 0.0f;
    if (IsKeyDown(KEY_W)) inputForward += 1.0f;
    if (IsKeyDown(KEY_S)) inputForward -= 1.0f;
    if (IsKeyDown(KEY_D)) inputRight += 1.0f;
    if (IsKeyDown(KEY_A)) inputRight -= 1.0f;

    /* Camera-relative directions (XZ only) */
    Vector3 camFwd = GAME_CAMERA_GetForwardXZ(&game->camera);
    Vector3 camRight = GAME_CAMERA_GetRightXZ(&game->camera);

    if (inputForward != 0.0f || inputRight != 0.0f)
    {
        /* Combine input into world-space move direction */
        Vector3 moveDir = {
            camFwd.x * inputForward + camRight.x * inputRight,
            0.0f,
            camFwd.z * inputForward + camRight.z * inputRight,
        };

        /* Normalize to prevent diagonal speed boost */
        float len = sqrtf(moveDir.x * moveDir.x + moveDir.z * moveDir.z);
        if (len > 0.0f)
        {
            moveDir.x /= len;
            moveDir.z /= len;
        }

        data->dummyPos.x += moveDir.x * data->dummySpeed * dt;
        data->dummyPos.z += moveDir.z * data->dummySpeed * dt;

        /*
         * FOLLOW: rotate dummy to face movement direction.
         * AIM:    dummy always faces camera forward (strafe movement).
         */
        if (game->camera.mode == CAMERA_MODE_FOLLOW)
        {
            data->dummyYaw = atan2f(moveDir.x, moveDir.z);
        }
    }

    /*
     * In AIM mode, the character always faces where the camera looks,
     * regardless of movement. This happens every tick (not just when moving)
     * so the character tracks the camera even while standing still.
     */
    if (game->camera.mode == CAMERA_MODE_AIM)
    {
        data->dummyYaw = atan2f(camFwd.x, camFwd.z);
    }

    /* Update dummy transform */
    RENDERER_SetTransform(&game->renderer, data->dummyHandle,
        MatrixMultiply(
            MatrixRotateY(data->dummyYaw),
            MatrixTranslate(data->dummyPos.x, 0.5f, data->dummyPos.z)
        ));

    /* Sync capsule collider with dummy position */
    PHYS_WORLD_SetPosition(&game->physWorld, data->dummyColl,
        (Vector3){ data->dummyPos.x, 0.5f, data->dummyPos.z });

    /* Feed camera target (smoothing happens in GAME_CAMERA_Update per frame) */
    GAME_CAMERA_SetTarget(&game->camera, data->dummyPos);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Render3D
 * ═══════════════════════════════════════════════════════════════════════════ */

static void Render3D(Game* game, float alpha)
{
    (void)game;
    (void)alpha;
    DrawGrid(20, 1.0f);

    /* Facing direction arrow on the dummy */
    if (data)
    {
        float arrowLen = 1.5f;
        float yaw = data->dummyYaw;
        Vector3 base = { data->dummyPos.x, 0.05f, data->dummyPos.z };
        Vector3 dir = { sinf(yaw) * arrowLen, 0.0f, cosf(yaw) * arrowLen };
        Vector3 tip = Vector3Add(base, dir);

        /* Shaft */
        DrawLine3D(base, tip, YELLOW);

        /* Arrowhead: small cone pointing in the facing direction */
        Vector3 coneBase = Vector3Add(base, (Vector3) {
            dir.x * 0.75f, 0.0f, dir.z * 0.75f
        });
        DrawCylinderEx(coneBase, tip, 0.15f, 0.0f, 6, YELLOW);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  RenderHUD
 * ═══════════════════════════════════════════════════════════════════════════ */

static void RenderHUD(Game* game, float alpha)
{
    (void)alpha;

    const char* modeStr = (game->camera.mode == CAMERA_MODE_AIM) ? "AIM" : "FOLLOW";

    DrawText("LEVEL A — Orbit camera", 10, SCREEN_HEIGHT - 110, 20, RAYWHITE);
    DrawText(TextFormat("Drawn: %d  Culled: %d",
        game->renderer.statsDrawn, game->renderer.statsCulled),
        10, SCREEN_HEIGHT - 85, 16, GREEN);
    DrawText(TextFormat("Camera: %s", modeStr), 10, SCREEN_HEIGHT - 63, 16, YELLOW);
    DrawText("WASD: move  |  Mouse: orbit  |  RMB: aim", 10, SCREEN_HEIGHT - 43, 16, LIGHTGRAY);
    DrawText("F1: debug  |  F4: physics wireframes", 10, SCREEN_HEIGHT - 43, 16, LIGHTGRAY);
    DrawText("SPACE: Level B", 10, SCREEN_HEIGHT - 23, 16, LIGHTGRAY);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Level Definition
 * ═══════════════════════════════════════════════════════════════════════════ */

Level LEVEL_TEST_A = {
    .name = "Test A (camera)",
    .Init = Init,
    .Shutdown = Shutdown,
    .ProcessInput = ProcessInput,
    .Update = Update,
    .Render3D = Render3D,
    .RenderHUD = RenderHUD,
};