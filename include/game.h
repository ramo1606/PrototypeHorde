#pragma once
#include "raylib.h"
#include "memory.h"
#include "level_manager.h"
#include "renderer.h"
#include "physics_world.h"
#include <stdbool.h>

typedef struct Actor Actor;
typedef struct Level Level;
typedef struct Game Game;

#define SCREEN_WIDTH   1024
#define SCREEN_HEIGHT  768
#define GAME_TITLE  "Prototype Horde"

#define UPDATE_RATE 60
#define FIXED_TIMESTEP (1.0f / (float)UPDATE_RATE)
#define MAX_DELTA_TIME 0.25f
#define RENDER_FPS 30

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
    GameState state;

    float accumulator;
    int updateCount;

    MemorySystem memory;
    LevelManager levelMgr;
    Renderer renderer;
    PhysWorld physWorld;

    /* InputSystem  input;   */         /* Phase 1 */
    /* EventSystem  events;  */         /* Phase 2 */
    /* AudioSystem  audio;   */         /* Phase 8 */

    Actor *actors[GAME_MAX_ACTORS];
    int    actorCount;
    Actor *pendingActors[GAME_MAX_PENDING];
    int    pendingCount;
    bool updatingActors;
    int actorsCreated;
};

bool GAME_Init(Game* game, Level* initialLevel);
void GAME_Shutdown(Game* game);
void GAME_Run(Game* game);

void GAME_AddActor(Game* game, Actor* actor);
void GAME_RemoveActor(Game* game, Actor* actor);
void GAME_RemoveAllActors(Game* game);

void GAME_ChangeLevel(Game* game, Level* level);

Actor* GAME_FindActorByTag(Game* game, unsigned int tag);
int GAME_FindActorsByTag(Game* game, unsigned int tag, Actor** outArray, int maxResults);

float GAME_GetTime(Game* game);