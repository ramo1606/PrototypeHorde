#include "level.h"
#include "game.h"
#include "actor.h"
#include "component.h"
#include "mesh_component.h"
#include "move_component.h"
#include "camera_tps.h"
#include <assert.h>
#include <math.h>

enum
{
    L5_MESH_CUBE = 0,
    L5_MESH_SPHERE,
    L5_MESH_CYLINDER,
    L5_MESH_FLOOR,
    L5_MESH_COUNT
};

static Mesh     SMeshes[L5_MESH_COUNT];
static Material SMaterial;

static void LoadResources(void)
{
    SMeshes[L5_MESH_CUBE] = GenMeshCube(1, 1, 1);
    SMeshes[L5_MESH_SPHERE] = GenMeshSphere(0.5f, 16, 16);
    SMeshes[L5_MESH_CYLINDER] = GenMeshCylinder(0.3f, 3, 16);
    SMeshes[L5_MESH_FLOOR] = GenMeshPlane(50, 50, 1, 1);
    SMaterial = LoadMaterialDefault();
}

static void UnloadResources(void)
{
    for (int i = 0; i < L5_MESH_COUNT; i++)
    {
        UnloadMesh(SMeshes[i]);
    }
    UnloadMaterial(SMaterial);
}

/* ── Player input ──────────────────────────────────────────────── */

static void PlayerInput(Actor* self)
{
    assert(self != NULL);
    Component* comp = ACTOR_GetComponentOfType(self, COMPONENT_MOVE);
    if (!comp) return;
    MoveComponent* mc = (MoveComponent*)comp;

    float forward = 0.0f;
    float angular = 0.0f;
    float strafe = 0.0f;

    if (IsKeyDown(KEY_W)) forward += 8.0f;
    if (IsKeyDown(KEY_S)) forward -= 8.0f;
    if (IsKeyDown(KEY_A)) angular += PI;
    if (IsKeyDown(KEY_D)) angular -= PI;
    if (IsKeyDown(KEY_Q)) strafe -= 5.0f;
    if (IsKeyDown(KEY_E)) strafe += 5.0f;

    mc->forwardSpeed = forward;
    mc->angularSpeed = angular;
    mc->strafeSpeed = strafe;
}

/* ── Level hooks ───────────────────────────────────────────────── */

extern Level LEVEL_1;

static Actor* SPlayer = NULL;

static void LEVEL_5_Init(Game* game)
{
    assert(game != NULL);
    SetRandomSeed(GetTime());
    LoadResources();

    /* Floor */
    {
        Actor* a = ACTOR_Create(game);
        if (!a) return;
        ACTOR_SetPosition(a, (Vector3) { 0, -0.001f, 0 });
        MeshComponent* mc = MESH_COMPONENT_Create(a, &SMeshes[L5_MESH_FLOOR], &SMaterial);
        mc->tint = (Color){ 50, 50, 50, 255 };
    }

    /* Pillars for spatial reference */
    Color pillarColors[] = { RED, BLUE, GREEN, YELLOW, PURPLE, ORANGE };
    for (int i = 0; i < 12; i++)
    {
        Actor* a = ACTOR_Create(game);
        if (!a) continue;
        float angle = (float)i * (2.0f * PI / 12.0f);
        float radius = 15.0f;
        ACTOR_SetPosition(a, (Vector3) {
            cosf(angle)* radius, 1.5f, sinf(angle)* radius
        });
        MeshComponent* mc = MESH_COMPONENT_Create(a, &SMeshes[L5_MESH_CYLINDER], &SMaterial);
        mc->tint = pillarColors[i % 6];
    }

    /* Player */
    {
        SPlayer = ACTOR_Create(game);
        if (!SPlayer) return;
        ACTOR_SetPosition(SPlayer, (Vector3) { 0, 0.5f, 0 });

        MeshComponent* mc = MESH_COMPONENT_Create(SPlayer, &SMeshes[L5_MESH_CUBE], &SMaterial);
        mc->tint = GREEN;

        MOVE_COMPONENT_Create(SPlayer);
        CameraTPS* ctps = CAMERA_TPS_Create(SPlayer);
        CAMERA_COMPONENT_Apply(&ctps->cameraComponent);
        SPlayer->Input = PlayerInput;
    }

    /* Wandering spheres */
    Color wanderColors[] = { RED, ORANGE, SKYBLUE, PURPLE, YELLOW, PINK };
    for (int i = 0; i < 6; i++)
    {
        Actor* a = ACTOR_Create(game);
        if (!a) continue;
        float angle = (float)i * (2.0f * PI / 6.0f);
        ACTOR_SetPosition(a, (Vector3) {
            cosf(angle) * 8.0f, 0.5f, sinf(angle) * 8.0f
        });

        MeshComponent* mc = MESH_COMPONENT_Create(a, &SMeshes[L5_MESH_SPHERE], &SMaterial);
        mc->tint = wanderColors[i];

        MoveComponent* mv = MOVE_COMPONENT_Create(a);
        mv->forwardSpeed = 1.0f + (float)GetRandomValue(-10, 10) / 10.0f;
        mv->angularSpeed = 0.5f + (float)GetRandomValue(-10, 10) / 10.0f;
    }
}

static void LEVEL_5_Shutdown(Game* game)
{
    assert(game != NULL);
    (void)game;
    SPlayer = NULL;
    UnloadResources();
}

static void LEVEL_5_Input(Game* game)
{
    assert(game != NULL);
    if (game->state != GAME_STATE_GAMEPLAY) return;

    /* Spawn wanderer */
    if (IsKeyPressed(KEY_SPACE))
    {
        Actor* a = ACTOR_Create(game);
        if (a)
        {
            float x = (float)GetRandomValue(-15, 15);
            float z = (float)GetRandomValue(-15, 15);
            ACTOR_SetPosition(a, (Vector3) { x, 0.5f, z });

            Color colors[] = { RED, ORANGE, YELLOW, PURPLE, SKYBLUE, PINK };
            MeshComponent* mc = MESH_COMPONENT_Create(a, &SMeshes[L5_MESH_SPHERE], &SMaterial);
            mc->tint = colors[GetRandomValue(0, 5)];

            MoveComponent* mv = MOVE_COMPONENT_Create(a);
            mv->forwardSpeed = 1.0f + (float)GetRandomValue(-15, 15) / 10.0f;
            mv->angularSpeed = 0.5f + (float)GetRandomValue(-15, 15) / 10.0f;
        }
    }

    /* Kill last non-player actor */
    if (IsKeyPressed(KEY_BACKSPACE) && game->actorCount > 2)
    {
        game->actors[game->actorCount - 1]->state = ACTOR_STATE_DEAD;
    }

    if (IsKeyPressed(KEY_TAB))
    {
        GAME_ChangeLevel(game, &LEVEL_1);
    }
}

static void LEVEL_5_Render3D(Game* game)
{
    assert(game != NULL);
    DrawGrid(50, 1.0f);

    /* Player axes */
    if (SPlayer && SPlayer->state == ACTOR_STATE_ACTIVE)
    {
        Vector3 p = ACTOR_GetWorldPosition(SPlayer);
        Vector3 fwd = ACTOR_GetForward(SPlayer);
        Vector3 rgt = ACTOR_GetRight(SPlayer);

        DrawLine3D(p, Vector3Add(p, Vector3Scale(fwd, 2.0f)), YELLOW);
        DrawLine3D(p, Vector3Add(p, Vector3Scale(rgt, 1.5f)), RED);
        DrawLine3D(p, Vector3Add(p, (Vector3) { 0, 1.5f, 0 }), GREEN);
    }

    /* Wanderer forward lines */
    for (int i = 0; i < game->actorCount; i++)
    {
        Actor* a = game->actors[i];
        if (a == SPlayer || a->state != ACTOR_STATE_ACTIVE) continue;
        if (!ACTOR_GetComponentOfType(a, COMPONENT_MOVE)) continue;

        Vector3 fwd = ACTOR_GetForward(a);
        Vector3 pos = ACTOR_GetWorldPosition(a);
        DrawLine3D(pos, Vector3Add(pos, Vector3Scale(fwd, 1.0f)), YELLOW);
    }
}

static int LEVEL_5_RenderHUD(Game* game, int y)
{
    assert(game != NULL);
    const int step = 22;

    DrawText("Level 5 - FollowCamera", 10, y, 18, WHITE);
    y += step + 4;

    DrawText("W/S - Forward/Back   A/D - Rotate   Q/E - Strafe", 10, y, 16, LIGHTGRAY);
    y += step;
    DrawText("SPACE - Spawn wanderer   BACKSPACE - Kill last", 10, y, 16, LIGHTGRAY);
    y += step;
    DrawText("TAB - Next level (cycles to 1)", 10, y, 16, YELLOW);
    y += step;

    if (SPlayer)
    {
        Vector3 pos = ACTOR_GetWorldPosition(SPlayer);
        const char* posText = TextFormat("Pos: %.1f, %.1f, %.1f",
            pos.x, pos.y, pos.z);
        DrawText(posText, 10, y, 16, GREEN);
        y += step;

        Vector3 fwd = ACTOR_GetForward(SPlayer);
        const char* dir = TextFormat("Fwd: %.2f, %.2f, %.2f", fwd.x, fwd.y, fwd.z);
        DrawText(dir, 10, y, 16, YELLOW);
        y += step;

        /* Camera info */
        Camera3D cam = game->renderer.camera;
        const char* camPos = TextFormat("Cam: %.1f, %.1f, %.1f",
            cam.position.x, cam.position.y, cam.position.z);
        DrawText(camPos, 10, y, 16, SKYBLUE);
        y += step;
    }

    (void)game;
    return y;
}

Level LEVEL_5 =
{
    .name = "5 - CameraTPS",
    .Init = LEVEL_5_Init,
    .Shutdown = LEVEL_5_Shutdown,
    .ProcessInput = LEVEL_5_Input,
    .Render3D = LEVEL_5_Render3D,
    .RenderHUD = LEVEL_5_RenderHUD,
};