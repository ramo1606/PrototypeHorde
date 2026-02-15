#include "game.h"
#include "scene.h"
#include <stdlib.h>

extern Scene SCENE_1;
extern Scene SCENE_2;
extern Scene SCENE_3;
extern Scene SCENE_4;

/* Main game loop */
int main(void) 
{
    Game* game = (Game*)malloc(sizeof(Game));
    if (!game) 
    {
		TraceLog(LOG_ERROR, "Failed to allocate memory for Game struct");
        return -1;
    }
    
    if (!GAME_Init(game, &SCENE_1)) 
    {
        free(game);
        return -1;
    }

    GAME_Run(game);
    GAME_Shutdown(game);

	free(game);

    return 0;
}