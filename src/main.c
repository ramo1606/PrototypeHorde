#include "game.h"
#include "level.h"
#include <stdlib.h>

extern Level LEVEL_1;
extern Level LEVEL_2;
extern Level LEVEL_3;
extern Level LEVEL_4;
extern Level LEVEL_5;

/* Main game loop */
int main(void) 
{
    /*
     * Allocate the Game struct on the heap so its large embedded arrays
     * (actors, meshes, etc.) do not exhaust the default stack size.
     * Init → Run → Shutdown is the entire lifecycle.
     */
    Game* game = (Game*)malloc(sizeof(Game));
    if (!game) 
    {
		TraceLog(LOG_ERROR, "Failed to allocate memory for Game struct");
        return -1;
    }
    
    if (!GAME_Init(game, &LEVEL_1)) 
    {
        free(game);
        return -1;
    }

    GAME_Run(game);
    GAME_Shutdown(game);

	free(game);

    return 0;
}