#include "level.h"
#include "game.h"
#include "actor.h"
#include "raylib.h"
#include <assert.h>
#include <stdlib.h>
#include <math.h>

extern Level LEVEL_3;

/* Colors assigned to actors for visual distinction */
static const Color s_colors[] = 
{ 
    { 230, 41, 55, 255 },      /* RED */
    { 0, 121, 241, 255 },      /* BLUE */
    { 0, 228, 48, 255 },       /* GREEN */
    { 255, 161, 0, 255 },      /* ORANGE */
    { 200, 122, 255, 255 },    /* PURPLE */
    { 102, 191, 255, 255 },    /* SKYBLUE */
    { 253, 249, 0, 255 },      /* YELLOW */
    { 255, 109, 194, 255 }     /* PINK */
};
#define COLOR_COUNT ((int)(sizeof(s_colors) / sizeof(s_colors[0])))

static int spawnCount = 0;

static void LEVEL_2_Init(Game* game) 
{
	assert(game != NULL);
	SetRandomSeed(GetTime());
    spawnCount = 0;

    Camera3D resetCamera = (Camera3D){
            .position = (Vector3){ 15.0f, 12.0f, 15.0f },
            .target = (Vector3){ 0.0f, 0.0f, 0.0f },
            .up = (Vector3){ 0.0f, 1.0f, 0.0f },
            .fovy = 45.0f,
            .projection = CAMERA_PERSPECTIVE,
    };
    RENDERER_SetCamera(&game->renderer, resetCamera);

    /* Pre-spawn a few actors */
    for (int i = 0; i < 4; i++) 
    {
        Actor* a = ACTOR_Create(game);
        float angle = (float)i * (2.0f * PI / 4.0f);
        ACTOR_SetPosition(a, (Vector3) { cosf(angle) * 4.0f, 0.5f, sinf(angle) * 4.0f });
        spawnCount++;
    }
}

static void LEVEL_2_Shutdown(Game* game) 
{
    (void)game;
    spawnCount = 0;
}

static void LEVEL_2_Input(Game* game) 
{
	assert(game != NULL);
    if (game->state != GAME_STATE_GAMEPLAY) return;

    if (IsKeyPressed(KEY_SPACE)) 
    {
        Actor* a = ACTOR_Create(game);
        if (a) 
        {
            float x = (float)GetRandomValue(-8, 7);
            float z = (float)GetRandomValue(-8, 7);
            ACTOR_SetPosition(a, (Vector3) { x, 0.5f, z });
            spawnCount++;
        }
    }

    if (IsKeyPressed(KEY_BACKSPACE) && game->actorCount > 0) 
    {
        game->actors[game->actorCount - 1]->state = ACTOR_STATE_DEAD;
    }

    if (IsKeyPressed(KEY_TAB)) 
    {
        GAME_ChangeLevel(game, &LEVEL_3);
    }
}

static void LEVEL_2_Render3D(Game * game)
{
    DrawGrid(30, 1.0f);
    /* Draw each actor as a wireframe cube � no MeshComponent involved */
    for (int i = 0; i < game->actorCount; i++) 
    {
        Actor* a = game->actors[i];
        if (a->state != ACTOR_STATE_ACTIVE) continue;

        Color c = s_colors[i % COLOR_COUNT];
        DrawCubeV(a->position, (Vector3) { 1, 1, 1 }, c);
        DrawCubeWiresV(a->position, (Vector3) { 1.01f, 1.01f, 1.01f }, BLACK);
    }
}

static int LEVEL_2_RenderHud(Game* game, int y) 
{
    const int step = 22;

    DrawText("Level 2 - Actor Lifecycle", 10, y, 18, WHITE);
    y += step + 4;

    const char* info = TextFormat("Actors: %d  (total spawned: %d)",
        game->actorCount, spawnCount);
    DrawText(info, 10, y, 16, GREEN);
    y += step;

    DrawText("SPACE - Spawn actor   BACKSPACE - Kill last", 10, y, 16, LIGHTGRAY);
    y += step;
    DrawText("TAB - Next level", 10, y, 16, YELLOW);
    y += step;

    return y;
}

Level LEVEL_2 = {
    .name = "2 - Actor Lifecycle",
    .Init = LEVEL_2_Init,
    .Shutdown = LEVEL_2_Shutdown,
    .ProcessInput = LEVEL_2_Input,
    .Render3D = LEVEL_2_Render3D,
    .RenderHUD = LEVEL_2_RenderHud
};