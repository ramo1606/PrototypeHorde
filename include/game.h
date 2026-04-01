#pragma once

#include "config.h"
#include "camera.h"
#include "physics.h"
#include "memory.h"
#include "level_manager.h"
#include "renderer.h"
#include "raylib.h"
#include <stdbool.h>

typedef struct Level Level;

typedef struct Game 
{
    /* Memory arenas (rmem MemPool by value) */
    MemoryArena permanent;        /* Lives for the entire program */
    MemoryArena level;            /* Reset on level transitions */
    MemoryArena scratch;          /* Reset every frame */

    /* Subsystems */
    LevelManager levelMgr;
    Renderer     renderer;
    PhysWorld    physWorld;        /* Physics world (Fase 2) */
    GameCamera   camera;

    /* Timing */
    float  accumulator;         /* Unprocessed time carried between frames */
    float  alpha;               /* Interpolation factor (0..1) */
    int    updateCount;         /* Fixed-steps executed this frame */

    /* Frametime stats (for debug) */
    float  frametimeMs;
    float  frametimeMin;
    float  frametimeMax;
    float  frametimeAvg;
    float  frametimeAccum;
    int    frametimeCount;
    double frametimeResetTimer;

    bool   running;
} Game;

/* ── Public API ──────────────────────────────────────────────────────────── */

bool GAME_Init(Game* game, Level* initialLevel);
void GAME_Shutdown(Game* game);
void GAME_Run(Game* game);