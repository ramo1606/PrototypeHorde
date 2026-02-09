#include "game.h"

/* Main game loop */
int main(void) 
{
    Game* game = (Game*)malloc(sizeof(Game));
    if (!game) 
    {
		TraceLog(LOG_ERROR, "Failed to allocate memory for Game struct");
        return -1;
    }

    bool success = GAME_Init(game);
    
    if (!GAME_Init(game)) 
    {
        free(game);
        return -1;
    }

    GAME_Run(game);
    GAME_Shutdown(game);

	free(game);

    return 0;
}