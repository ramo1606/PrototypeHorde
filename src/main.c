#include "game.h"
#include "level_sandbox.h"
#include <stdlib.h>

int main(void)
{
    Game* game = (Game*)malloc(sizeof(Game));
    if (!game)
    {
        TraceLog(LOG_ERROR, "Failed to allocate memory for Game structure");
        return -1;
    }

    if (!GameInit(game, &LEVEL_SANDBOX))
    {
        free(game);
        return -1;
    }

    GameRun(game);
    GameShutdown(game);

    free(game);

    return 0;
}
