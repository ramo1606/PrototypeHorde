/*******************************************************************************************
*
*   main.c — Application Entry Point
*
*   Minimal entry point that allocates the Game struct on the heap and runs
*   the standard Init → Run → Shutdown lifecycle. The Game struct is heap-
*   allocated because it's large (~50KB+ with all subsystems inline) and would
*   risk stack overflow on platforms with small default stack sizes.
*
*   Flow:
*       main()
*         ├── malloc(Game)           → Allocate game struct on heap
*         ├── GAME_Init(&LEVEL_1)   → Bootstrap engine, load first level
*         ├── GAME_Run()            → Enter main loop (blocks until quit)
*         ├── GAME_Shutdown()       → Tear down in reverse init order
*         └── free(Game)            → Release game struct
*
********************************************************************************************/
#include "game.h"
#include "level.h"
#include <stdlib.h>

/* ── Level declarations (defined in their respective level_N.c files) ────── */
extern Level LEVEL_SANDBOX;

/*------------------------------------------------------------------------------------
 * main
 *
 *   Standard C entry point. Returns 0 on success, -1 on allocation or
 *   initialization failure. All engine teardown is handled by GAME_Shutdown
 *   so we only need to free the Game struct itself here.
 *------------------------------------------------------------------------------------*/

int main(void) 
{
    Game* game = (Game*)malloc(sizeof(Game));
    if (!game) 
    {
		TraceLog(LOG_ERROR, "Failed to allocate memory for Game struct");
        return -1;
    }
    
    if (!GAME_Init(game, &LEVEL_SANDBOX)) 
    {
        free(game);
        return -1;
    }

    GAME_Run(game);
    GAME_Shutdown(game);

	free(game);

    return 0;
}