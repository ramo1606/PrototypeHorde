#include "scene.h"
#include "game.h"
#include <assert.h>
#include <stddef.h>

extern Scene SCENE_2;

static void SCENE_1_Input(Game* game) 
{
    assert(game != NULL);

    if (game->state != GAME_STATE_GAMEPLAY) return;

    if (IsKeyPressed(KEY_TAB)) 
    {
        GAME_ChangeScene(game, &SCENE_2);
    }
}

static int SCENE_1_RenderHud(Game* game, int y) 
{
    const int step = 22;
    (void)game;

    DrawText("Scene 1 - Debug Overlay", 10, y, 18, WHITE);
    y += step + 4;

    DrawText("F1 - Toggle debug overlay", 10, y, 16, LIGHTGRAY);
    y += step;
    DrawText("TAB - Next scene", 10, y, 16, YELLOW);
    y += step;

    return y;
}

Scene SCENE_1 = 
{
    .name = "1 - Debug Overlay",
    .Init = NULL,
    .Shutdown = NULL,
    .ProcessInput = SCENE_1_Input,
    .Render3D = NULL,
    .RenderHUD = SCENE_1_RenderHud,
};