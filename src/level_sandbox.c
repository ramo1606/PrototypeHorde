/*
 * level_sandbox.c — Default level used as scratch ground for renderer,
 *                   physics, camera, and (later) gameplay systems.
 *
 * What lives here for now:
 *   - A capsule "dummy" that stands in for the player (WASD moves it).
 *   - A reference cube at origin and three obstacle boxes for collision.
 *   - An orbiting marker sphere that exercises frustum culling.
 *
 * The dummy uses world-axis movement (WASD = world XZ) — it's NOT the
 * final 8-direction input system. Phase 3.1 introduces the real input
 * abstraction; this is just enough to exercise PhysicsMoveAndCollide
 * and the camera follow.
 */

#include "level_sandbox.h"
#include "game.h"
#include "arena.h"
#include "config.h"
#include "layers.h"
#include "level_manager.h"
#include "renderer.h"
#include "camera.h"
#include "raylib.h"
#include "raymath.h"
#include <math.h>

typedef struct
{
    /* Orbiting marker (tests frustum culling). */
    float        rotation;
    float        rotationPrev;
    Model        markerModel;
    RenderHandle markerHandle;

    /* Dummy target (placeholder for the player). */
    Vector3        dummyPos;
    float          dummyYaw;
    float          dummySpeed;
    Model          dummyModel;
    RenderHandle   dummyHandle;
    ColliderHandle dummyColl;

    /* Static reference cube at origin. */
    Model          cubeModel;
    RenderHandle   cubeHandle;
    ColliderHandle cubeColl;

    /* Obstacle boxes. */
    Model          obstacleModel;
    RenderHandle   obstacleHandles[3];
    ColliderHandle obstacleColl[3];
} SandboxData;

static SandboxData* data = NULL;

/* ── Lifecycle ───────────────────────────────────────────────────────────── */

static void Init(void* user)
{
    Game* game = (Game*)user;

    data = ARENA_ALLOC(&game->level, SandboxData);

    /* Orbiting marker. */
    data->rotation     = 0.0f;
    data->rotationPrev = 0.0f;
    data->markerModel  = LoadModelFromMesh(GenMeshSphere(0.2f, 8, 8));
    GameApplyDefaultShader(game, &data->markerModel);
    data->markerHandle = RendererRegister(&game->renderer, data->markerModel, 0);

    /* Reference cube at origin. */
    data->cubeModel  = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    GameApplyDefaultShader(game, &data->cubeModel);
    data->cubeHandle = RendererRegister(&game->renderer, data->cubeModel, 0);
    RendererSetTransform(&game->renderer, data->cubeHandle,
        MatrixTranslate(0.0f, 0.5f, 0.0f));
    GameSetBlobShadow(game, data->cubeHandle, true, 0.7f);
    data->cubeColl = PhysicsAddBox(&game->physWorld,
        (Vector3){ 0.0f, 0.5f, 0.0f },
        (Vector3){ 0.5f, 0.5f, 0.5f },
        LAYER_SCENERY, MASK_ALL, -1,
        false, false);

    /* Dummy. */
    data->dummyPos    = (Vector3){ 0.0f, 0.0f, 3.0f };
    data->dummyYaw    = 0.0f;
    data->dummySpeed  = 5.0f;
    data->dummyModel  = LoadModelFromMesh(GenMeshCube(0.4f, 0.9f, 0.4f));
    GameApplyDefaultShader(game, &data->dummyModel);
    data->dummyHandle = RendererRegister(&game->renderer, data->dummyModel, 0);
    RendererSetTransform(&game->renderer, data->dummyHandle,
        MatrixTranslate(data->dummyPos.x, 0.45f, data->dummyPos.z));
    GameSetBlobShadow(game, data->dummyHandle, true, 0.5f);
    data->dummyColl = PhysicsAddCapsule(&game->physWorld,
        (Vector3){ data->dummyPos.x, 0.5f, data->dummyPos.z },
        0.3f, 0.2f,
        LAYER_PLAYER, LAYER_SCENERY | LAYER_ENEMY, 0,
        true, false);

    /* Obstacles. */
    data->obstacleModel = LoadModelFromMesh(GenMeshCube(2.0f, 1.0f, 1.0f));
    GameApplyDefaultShader(game, &data->obstacleModel);

    Vector3 obstaclePositions[3] = {
        {  4.0f, 0.5f,  0.0f },
        { -3.0f, 0.5f,  2.0f },
        {  1.0f, 0.5f, -4.0f },
    };

    for (int i = 0; i < 3; i++)
    {
        data->obstacleHandles[i] = RendererRegister(&game->renderer, data->obstacleModel, 0);
        RendererSetTransform(&game->renderer, data->obstacleHandles[i],
            MatrixTranslate(obstaclePositions[i].x, obstaclePositions[i].y, obstaclePositions[i].z));
        GameSetBlobShadow(game, data->obstacleHandles[i], true, 1.2f);

        data->obstacleColl[i] = PhysicsAddBox(&game->physWorld,
            obstaclePositions[i],
            (Vector3){ 1.0f, 0.5f, 0.5f },
            LAYER_SCENERY, MASK_ALL, -1,
            false, false);
    }

    CameraSetTarget(&game->camera, data->dummyPos);
    GameSetClearColor(game, (Color) { 25, 40, 80, 255 });
}

static void Shutdown(void* user)
{
    Game* game = (Game*)user;
    if (!data) return;

    PhysicsRemove(&game->physWorld, data->dummyColl);
    PhysicsRemove(&game->physWorld, data->cubeColl);
    for (int i = 0; i < 3; i++) PhysicsRemove(&game->physWorld, data->obstacleColl[i]);

    RendererUnregister(&game->renderer, data->markerHandle);
    RendererUnregister(&game->renderer, data->cubeHandle);
    RendererUnregister(&game->renderer, data->dummyHandle);
    for (int i = 0; i < 3; i++) RendererUnregister(&game->renderer, data->obstacleHandles[i]);

    UnloadModel(data->markerModel);
    UnloadModel(data->cubeModel);
    UnloadModel(data->dummyModel);
    UnloadModel(data->obstacleModel);

    data = NULL;
}

/* ── Update (fixed timestep) ─────────────────────────────────────────────── */

static void Update(void* user, float dt)
{
    Game* game = (Game*)user;

    /* Orbiting marker. */
    data->rotationPrev = data->rotation;
    data->rotation += 90.0f * dt;
    if (data->rotation > 360.0f)
    {
        data->rotation     -= 360.0f;
        data->rotationPrev -= 360.0f;
    }
    float rad = data->rotation * DEG2RAD;
    RendererSetTransform(&game->renderer, data->markerHandle,
        MatrixTranslate(cosf(rad) * 2.0f, 0.5f, sinf(rad) * 2.0f));

    /* Dummy movement: WASD on world XZ. Placeholder until Phase 3.1. */
    float ix = 0.0f, iz = 0.0f;
    if (IsKeyDown(KEY_W)) iz -= 1.0f;
    if (IsKeyDown(KEY_S)) iz += 1.0f;
    if (IsKeyDown(KEY_A)) ix -= 1.0f;
    if (IsKeyDown(KEY_D)) ix += 1.0f;

    if (ix != 0.0f || iz != 0.0f)
    {
        Vector3 moveDir = { ix, 0.0f, iz };
        float len = Vector3Length(moveDir);
        if (len > 0.0f) moveDir = Vector3Scale(moveDir, 1.0f / len);

        Vector3 delta    = Vector3Scale(moveDir, data->dummySpeed * dt);
        Vector3 finalPos = PhysicsMoveAndCollide(&game->physWorld,
            data->dummyColl, delta, LAYER_SCENERY, NULL);

        data->dummyPos.x = finalPos.x;
        data->dummyPos.z = finalPos.z;
        data->dummyYaw   = atan2f(moveDir.x, moveDir.z);
    }

    RendererSetTransform(&game->renderer, data->dummyHandle,
        MatrixMultiply(
            MatrixRotateY(data->dummyYaw),
            MatrixTranslate(data->dummyPos.x, 0.45f, data->dummyPos.z)
        ));

    CameraSetTarget(&game->camera, data->dummyPos);
}

static void Render3D(void* user, float alpha)
{
    (void)user;
    (void)alpha;
    DrawGrid(20, 1.0f);
}

static void RenderHUD(void* user, float alpha)
{
    (void)user;
    (void)alpha;
    DrawText("SANDBOX", 10, SCREEN_HEIGHT - 35, 16, LIGHTGRAY);
    DrawText("WASD: move dummy", 10, SCREEN_HEIGHT - 55, 14, GRAY);
}

Level LEVEL_SANDBOX = {
    .name      = "Sandbox",
    .Init      = Init,
    .Shutdown  = Shutdown,
    .Update    = Update,
    .Render3D  = Render3D,
    .RenderHUD = RenderHUD,
};
