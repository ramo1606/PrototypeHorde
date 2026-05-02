#pragma once

#include "arena_types.h"
#include "camera_types.h"
#include "physics_types.h"
#include "renderer_types.h"
#include "level_manager_types.h"
#include <stdbool.h>

typedef struct Game
{
    /* Memory arenas (rmem MemPool by value). See docs/modules/arena.md. */
    MemArena permanent;       /* Lives for the entire program           */
    MemArena level;            /* Reset on level transitions             */
    MemArena scratch;          /* Reset every frame                      */

    /* Subsystems */
    LevelManager levelMgr;
    Renderer     renderer;
    PhysWorld    physWorld;
    GameCamera   camera;

    /* Timing */
    float accumulator;         /* Unprocessed time carried between frames */
    float alpha;               /* Interpolation factor (0..1)             */
    int   updateCount;         /* Fixed-steps executed this frame         */

    /* Frametime stats (debug overlay) */
    float  frametimeMs;
    float  frametimeMin;
    float  frametimeMax;
    float  frametimeAvg;
    float  frametimeAccum;
    int    frametimeCount;
    double frametimeResetTimer;

    bool running;
} Game;
