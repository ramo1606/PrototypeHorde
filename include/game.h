#pragma once
#include "raylib.h"
#include "memory.h"
#include <stdbool.h>

typedef struct Actor Actor;
typedef struct Scene Scene;
typedef struct Game Game;

#define SCREEN_WIDTH   1024
#define SCREEN_HEIGHT  768
#define GAME_TITLE  "Prototype Horde"

#define UPDATE_RATE 60
#define FIXED_TIMESTEP (1.0f / (float)UPDATE_RATE)  /* 60 updates per second */
#define MAX_DELTA_TIME 0.25f                 /* Prevent spiral of death */
#define RENDER_FPS 30						/* Change to 30 for weak hardware */

#define GAME_MAX_ACTORS     512
#define GAME_MAX_PENDING    64

typedef enum
{
    GAME_STATE_GAMEPLAY,
    GAME_STATE_PAUSED,
	GAME_STATE_QUIT
} GameState;

struct Game
{
	/* State */
    GameState state;

	/* Timing */
    float accumulator;
    int updateCount;

    /* Memory */
    MemorySystem memory;

	/* Actors */
    Actor *actors[GAME_MAX_ACTORS];
    int    actorCount;
    Actor *pendingActors[GAME_MAX_PENDING];
    int    pendingCount;
    bool updatingActors;

    int actorsCreated;

    /* Scene */
    Scene* activeScene;
    Scene* nextScene;
};

bool GAME_Init(Game* game, Scene* initialScene);
void GAME_Shutdown(Game* game);
void GAME_Run(Game* game);

void GAME_AddActor(Game* game, Actor* actor);
void GAME_RemoveActor(Game* game, Actor* actor);
void GAME_RemoveActiveActorByIndex(Game* game, int idx);
void GAME_RemovePendingActorByIndex(Game* game, int idx);
void GAME_RemoveAllActors(Game* game);

void GAME_ChangeScene(Game* game, Scene* scene);