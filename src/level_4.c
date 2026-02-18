#include "level.h"
#include "game.h"
#include "actor.h"
#include "component.h"
#include "mesh_component.h"
#include "move_component.h"
#include <assert.h>
#include <stdlib.h>
#include <math.h>

enum 
{
    MESH_CUBE = 0, 
    MESH_SPHERE, 
    MESH_FLOOR, 
    MESH_COUNT 
};

static Mesh     SMeshes[MESH_COUNT];
static Material SMaterial;

static void LoadResources(void) 
{
    SMeshes[MESH_CUBE] = GenMeshCube(1, 1, 1);
    SMeshes[MESH_SPHERE] = GenMeshSphere(0.5f, 16, 16);
    SMeshes[MESH_FLOOR] = GenMeshPlane(30, 30, 1, 1);
    SMaterial = LoadMaterialDefault();
}

static void UnloadResources(void) 
{
    for (int i = 0; i < MESH_COUNT; i++) 
    {
        UnloadMesh(SMeshes[i]);
    }
    UnloadMaterial(SMaterial);
}

static void PlayerInput(Actor* self) 
{
	assert(self != NULL);
    Component* comp = ACTOR_GetComponentOfType(self, COMPONENT_MOVE);
    if (!comp) return;
	assert(comp->type == COMPONENT_MOVE);
    MoveComponent* mc = (MoveComponent*)comp;

    float forward = 0.0f;
    float angular = 0.0f;
    float strafe = 0.0f;

    if (IsKeyDown(KEY_W)) forward += 6.0f;
    if (IsKeyDown(KEY_S)) forward -= 6.0f;
    if (IsKeyDown(KEY_A)) angular += PI;
    if (IsKeyDown(KEY_D)) angular -= PI;
    if (IsKeyDown(KEY_Q)) strafe -= 4.0f;
    if (IsKeyDown(KEY_E)) strafe += 4.0f;

	mc->forwardSpeed = forward;
	mc->angularSpeed = angular;
	mc->strafeSpeed = strafe;
}

extern Level LEVEL_5;

static Actor* SPlayer = NULL;

static void LEVEL_4_Init(Game* game) 
{
	assert(game != NULL);
	SetRandomSeed(GetTime());
    LoadResources();

    /* Floor */
    {
        Actor* a = ACTOR_Create(game);
        if (!a) return;
        ACTOR_SetPosition(a, (Vector3) { 0, -0.001f, 0 });
        MeshComponent* mc = MESH_COMPONENT_Create(a, &SMeshes[MESH_FLOOR], &SMaterial);
		mc->tint = GRAY;
    }

    /* Player */
    {
        SPlayer = ACTOR_Create(game);
        if(!SPlayer) return;
        ACTOR_SetPosition(SPlayer, (Vector3) { 0, 0.5f, 0 });

        MeshComponent* mc = MESH_COMPONENT_Create(SPlayer, &SMeshes[MESH_CUBE], &SMaterial);
        mc->tint = GREEN;

        MOVE_COMPONENT_Create(SPlayer);
        SPlayer->Input = PlayerInput;
    }

    /* Pre-spawn a few wanderers */
    Color colors[] = { RED, BLUE, ORANGE, PURPLE, SKYBLUE, YELLOW };
    for (int i = 0; i < 4; i++) 
    {
        Actor* a = ACTOR_Create(game);
        if (!a) return;
        float angle = (float)i * (2.0f * PI / 4.0f);
        ACTOR_SetPosition(a, (Vector3) 
        {
            cosf(angle) * 6.0f,
                0.5f,
                sinf(angle) * 6.0f
        });

        MeshComponent* mc = MESH_COMPONENT_Create(a, &SMeshes[MESH_SPHERE], &SMaterial);
		mc->tint = colors[i % 6];
        
        MoveComponent* mv = MOVE_COMPONENT_Create(a);
		mv->forwardSpeed = 1.0f + (float)GetRandomValue(-10, 10) / 10.0f;
		mv->angularSpeed = 0.5f + (float)GetRandomValue(-10, 10) / 10.0f;
    }
}

static void LEVEL_4_Shutdown(Game* game) 
{
	assert(game != NULL);
    (void)game;
    SPlayer = NULL;
    UnloadResources();
}

static void LEVEL_4_Input(Game* game) 
{
	assert(game != NULL);
    if (game->state != GAME_STATE_GAMEPLAY) return;

    /* Spawn wandering actor */
    if (IsKeyPressed(KEY_SPACE)) 
    {
        Actor* a = ACTOR_Create(game);
        if (a) 
        {
			float x = (float)GetRandomValue(-10, 10);
			float z = (float)GetRandomValue(-10, 10);
            ACTOR_SetPosition(a, (Vector3) { x, 0.5f, z });

            Color colors[] = { RED, ORANGE, YELLOW, PURPLE, SKYBLUE, PINK };
            MeshComponent* mc = MESH_COMPONENT_Create(a, &SMeshes[MESH_SPHERE], &SMaterial);
			mc->tint = colors[rand() % 6];

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
        GAME_ChangeLevel(game, &LEVEL_5);
    }
}

static void LEVEL_4_Render3D(Game* game) 
{
	assert(game != NULL);
    DrawGrid(30, 1.0f);
    /* Draw axis lines for player */
    if (SPlayer && SPlayer->state == ACTOR_STATE_ACTIVE)
    {
        Vector3 p = ACTOR_GetWorldPosition(SPlayer);
        Vector3 fwd = ACTOR_GetForward(SPlayer);
        Vector3 right = ACTOR_GetRight(SPlayer);

        DrawLine3D(p, Vector3Add(p, Vector3Scale(fwd, 2.0f)), YELLOW);
        DrawLine3D(p, Vector3Add(p, Vector3Scale(right, 1.5f)), RED);
        DrawLine3D(p, Vector3Add(p, (Vector3) { 0, 1.5f, 0 }), GREEN);
    }

    /* Draw forward line for wanderers */
    for (int i = 0; i < game->actorCount; i++)
    {
        Actor* a = game->actors[i];
        if (a == SPlayer || a->state != ACTOR_STATE_ACTIVE) continue;
        if (!ACTOR_GetComponentOfType(a, COMPONENT_MOVE)) continue;

        Vector3 fwd = ACTOR_GetForward(a);
        Vector3 pos = ACTOR_GetWorldPosition(a);
        DrawLine3D(pos,
            Vector3Add(pos, Vector3Scale(fwd, 1.0f)),
            YELLOW);
    }
}

static int LEVEL_4_RenderHUD(Game* game, int y)
{
	assert(game != NULL);
    const int step = 22;

    DrawText("Level 4 - MoveComponent", 10, y, 18, WHITE);
    y += step + 4;

    DrawText("W/S - Forward/Back   A/D - Rotate   Q/E - Strafe", 10, y, 16, LIGHTGRAY);
    y += step;
    DrawText("SPACE - Spawn wanderer   BACKSPACE - Kill last", 10, y, 16, LIGHTGRAY);
    y += step;
    DrawText("TAB - Next level (cycles to 1)", 10, y, 16, YELLOW);
    y += step;

    if (SPlayer) {
        Vector3 pos = ACTOR_GetWorldPosition(SPlayer);
        const char* posText = TextFormat("Pos: %.1f, %.1f, %.1f",
            pos.x, pos.y, pos.z);
        DrawText(posText, 10, y, 16, GREEN);
        y += step;

        Vector3 fwd = ACTOR_GetForward(SPlayer);
        const char* dir = TextFormat("Fwd: %.2f, %.2f, %.2f", fwd.x, fwd.y, fwd.z);
        DrawText(dir, 10, y, 16, YELLOW);
        y += step;
    }

    (void)game;
    return y;
}

Level LEVEL_4 = 
{
    .name = "4 - MoveComponent",
    .Init = LEVEL_4_Init,
    .Shutdown = LEVEL_4_Shutdown,
    .ProcessInput = LEVEL_4_Input,
    .Render3D = LEVEL_4_Render3D,
    .RenderHUD = LEVEL_4_RenderHUD,
};
