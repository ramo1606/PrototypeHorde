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
    MemArena permanent;
    MemArena level;
    MemArena scratch;
    LevelManager levelMgr;
    Renderer renderer;
    PhysWorld physWorld;
    GameCamera camera;
    Color clearColor;
    float accumulator;
    float alpha;
    int updateCount;
    float frametimeMs;
    float frametimeMin;
    float frametimeMax;
    float frametimeAvg;
    float frametimeAccum;
    int frametimeCount;
    double frametimeResetTimer;
    bool running;
} Game;

bool GameInit(Game* game, Level* initialLevel);
void GameShutdown(Game* game);
void GameRun(Game* game);
void GameSetClearColor(Game* game, Color color);
