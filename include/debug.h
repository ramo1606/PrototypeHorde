#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct Game Game;

#define DEBUG_MAX_PANELS 6

typedef void (*DebugPanelDrawFn)(float x, float y, float w, float h);
typedef void (*DebugRender3DFn)(Game* game);

typedef struct DebugPerfStats
{
    float frametimeMs;
    float frametimeAvg;
    float frametimeMin;
    float frametimeMax;
    int fps;
    int ticksThisFrame;
    float alpha;
    size_t arenaPermanentTotal;
    size_t arenaPermanentFree;
    size_t arenaLevelTotal;
    size_t arenaLevelFree;
    size_t arenaScratchTotal;
    size_t arenaScratchFree;
    int renderableCount;
    int drawCount;
    int statsDrawn;
    int statsCulled;
    int colliderCount;
    int pairsChecked;
    int contactsFound;
    int triggersFound;
} DebugPerfStats;

#ifdef DEBUG_ENABLED

void DebugInit(void);
void DebugShutdown(void);

void DebugRegisterPanel(int slot, const char* name, DebugPanelDrawFn drawFn);
void DebugRegister3D(int slot, DebugRender3DFn renderFn);

void DebugUpdate(Game* game);
void DebugRender3D(Game* game);
void DebugRender(Game* game);
void DebugSetPerfStats(const DebugPerfStats* stats);
bool DebugIsVisible(void);

#else

#define DebugInit()
#define DebugShutdown()
#define DebugRegisterPanel(slot, name, fn)
#define DebugRegister3D(slot, fn)
#define DebugUpdate(game)
#define DebugRender3D(game)
#define DebugRender(game)
#define DebugSetPerfStats(stats)
#define DebugIsVisible() false

#endif
