#pragma once
#include "raylib.h"
#include <stdlib.h>
#include <stdbool.h>

typedef struct Actor Actor;
typedef struct Game Game;

#define SCREEN_WIDTH   640
#define SCREEN_HEIGHT  480
#define GAME_TITLE  "Prototype Horde"

#define UPDATE_RATE 60
#define FIXED_TIMESTEP (1.0f / (float)UPDATE_RATE)  /* 60 updates per second */
#define MAX_DELTA_TIME 0.25f                 /* Prevent spiral of death */
#define RENDER_FPS 30						/* Change to 30 for weak hardware */

#define GAME_MAX_ACTORS     512
#define GAME_MAX_PENDING    64

typedef enum
{
    GAME_STATE_INIT,
    GAME_STATE_MENU,
    GAME_STATE_GAMEPLAY,
    GAME_STATE_PAUSED,
    GAME_STATE_GAME_OVER,
	GAME_STATE_QUIT
} GameState;
struct Game
{
    GameState state;

    /* Timing */
    float accumulator;      /* Time not yet consumed by fixed updates */
    int   updateCount;     /* How many fixed updates ran this frame (debug) */

    /* Actors */
    Actor *actors[GAME_MAX_ACTORS];
    int    actorCount;

    Actor *pendingActors[GAME_MAX_PENDING];
    int    pendingCount;

    bool updatingActors;

    /* Debug */
    int actorsCreated;

};

bool GAME_Init(Game* game);
void GAME_Run(Game* game);
void GAME_Shutdown(Game* game);

void GAME_ProcessInput(Game* game);
void GAME_FixedUpdate(Game* game, float deltaTime);
void GAME_Render(Game* game);

void GAME_AddActor(Game* game, Actor* actor);
void GAME_RemoveActor(Game* game, Actor* actor);