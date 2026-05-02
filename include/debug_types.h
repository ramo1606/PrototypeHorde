#pragma once

#include <stddef.h>

#define DEBUG_ENABLED    1
#define DEBUG_MAX_PANELS 6

typedef struct Game Game;

typedef void (*DebugPanelDrawFn)(float x, float y, float w, float h);
typedef void (*DebugRender3DFn)(Game* game);

typedef struct DebugPerfStats
{
    /* Timing (Panel F2) */
    float frametimeMs, frametimeAvg, frametimeMin, frametimeMax;
    int   fps;
    int   ticksThisFrame;
    float alpha;

    /* Memory (Panel F2) */
    size_t arenaPermanentTotal, arenaPermanentFree;
    size_t arenaLevelTotal,     arenaLevelFree;
    size_t arenaScratchTotal,   arenaScratchFree;

    /* Renderer (Panel F3) */
    int renderableCount;
    int drawCount;
    int statsDrawn;
    int statsCulled;

    /* Physics (Panel F4) */
    int colliderCount;
    int pairsChecked;
    int contactsFound;
    int triggersFound;
} DebugPerfStats;
