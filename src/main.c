#include "game.h"
#include "level.h"
#include <stdlib.h>

/* Level declarations */
extern Level LEVEL_TEST_A;

int main(void)
{
    Game* game = (Game*)malloc(sizeof(Game));
    if (!game) 
    {
		TraceLog(LOG_ERROR, "Failed to allocate memory for Game structure");
        return -1;
    }

    if (!GAME_Init(game, &LEVEL_TEST_A))
    {
        free(game);
        return -1;
    }

    GAME_Run(game);
    GAME_Shutdown(game);

    free(game);

    return 0;
}
