#include "level.h"
#include "game.h"
#include <assert.h>
#include <stddef.h>

extern Level LEVEL_2;

static void LEVEL_1_Input(Game* game) 
{
    assert(game != NULL);

    if (game->state != GAME_STATE_GAMEPLAY) return;

    if (IsKeyPressed(KEY_TAB)) 
    {
        GAME_ChangeLevel(game, &LEVEL_2);
    }
}

static int LEVEL_1_RenderHud(Game* game, int y) 
{
    const int step = 22;
    (void)game;

    DrawText("Level 1 - Debug Overlay", 10, y, 18, WHITE);
    y += step + 4;

    DrawText("F1 - Toggle debug overlay", 10, y, 16, LIGHTGRAY);
    y += step;
    DrawText("TAB - Next level", 10, y, 16, YELLOW);
    y += step;

    return y;
}

Level LEVEL_1 = 
{
    .name = "1 - Debug Overlay",
    .Init = NULL,
    .Shutdown = NULL,
    .ProcessInput = LEVEL_1_Input,
    .Render3D = NULL,
    .RenderHUD = LEVEL_1_RenderHud,
};