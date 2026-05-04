#include "game.h"
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

    if (!GameInit(game, &LEVEL_TEST_A))
    {
        free(game);
        return -1;
    }

    GameRun(game);
    GameShutdown(game);

    free(game);

    return 0;
}
