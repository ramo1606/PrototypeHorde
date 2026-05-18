#pragma once

#include "arena.h"
#include "camera.h"
#include "config.h"
#include "level_manager.h"
#include "physics.h"
#include "renderer.h"
#include <stdbool.h>

typedef struct Game
{
    /* Memory arenas */
    MemArena permanent;
    MemArena level;
    MemArena scratch;

	/* Subsystems */
    LevelManager levelMgr;
    Renderer renderer;
    PhysWorld physWorld;

	/* Camera */
    GameCamera camera;
    Color clearColor;

	/* Timing */
    float accumulator;
    float alpha;
    int updateCount;

	/* Framerate stats (updated every second) */
    float frametimeMs;
    float frametimeMin;
    float frametimeMax;
    float frametimeAvg;
    float frametimeAccum;
    int frametimeCount;
    double frametimeResetTimer;

	/* Running flag */
    bool running;
} Game;

bool GameInit(Game* game, Level* initialLevel);
void GameShutdown(Game* game);
void GameRun(Game* game);
void GameSetClearColor(Game* game, Color color);
