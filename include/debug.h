#pragma once

#include "config.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct Game Game;

#define DEBUG_ENABLED 1

#ifdef DEBUG_ENABLED

#define DEBUG_MAX_PANELS 6

/* Callback types for panel registration */
typedef void (*DebugPanelDrawFn)(float x, float y, float w, float h);
typedef void (*DebugRender3DFn)(Game* game);

/* ── Lifecycle ───────────────────────────────────────────────────────────── */

void DEBUG_Init(void);
void DEBUG_Shutdown(void);

/* ── Panel Registration ──────────────────────────────────────────────────── */

/* Register a 2D overlay panel. slot 0..5 maps to F2..F7. */
void DEBUG_RegisterPanel(int slot, const char* name, DebugPanelDrawFn draw_fn);

/* Register a 3D gizmo renderer. slot 0..5 maps to F2..F7 (shared toggle). */
void DEBUG_Register3D(int slot, DebugRender3DFn render_fn);

/* ── Update & Render ─────────────────────────────────────────────────────── */

void DEBUG_Update(Game* game);
void DEBUG_Render3D(Game* game);    /* Call inside BeginMode3D */
void DEBUG_Render(Game* game);      /* Call after EndMode3D */

/* ── Perf Stats Feed ─────────────────────────────────────────────────────── */

typedef struct DebugPerfStats 
{
    /* Timing (Panel F2) */
    float frametimeMs, frametimeAvg, frametimeMin, frametimeMax;
    int fps;
    int ticksThisFrame;
    float alpha;

    /* Memory (Panel F2) */
    size_t arenaPermanentTotal, arenaPermanentFree;
    size_t arenaLevelTotal, arenaLevelFree;
    size_t arenaScratchTotal, arenaScratchFree;

    /* Renderer (Panel F3) */
    int renderableCount;     /* Active slots in the renderable pool   */
    int drawCount;           /* Entries in draw list (passed culling)  */
    int statsDrawn;          /* Actually submitted to GPU this frame   */
    int statsCulled;         /* Rejected by frustum culling            */

    /* Physics (Panel F4) */
    int    colliderCount;       /* Active colliders in pool               */
    int    pairsChecked;        /* Pairs tested this tick                 */
    int    contactsFound;       /* Contacts detected this tick            */
    int    triggersFound;       /* Trigger overlaps this tick             */
} DebugPerfStats;

void DEBUG_SetPerfStats(const DebugPerfStats* stats);

/* ── Queries ─────────────────────────────────────────────────────────────── */

bool DEBUG_IsVisible(void);

#else /* release: zero-cost no-ops */

#define DEBUG_Init()
#define DEBUG_Shutdown()
#define DEBUG_RegisterPanel(slot, name, fn)
#define DEBUG_Register3D(slot, fn)
#define DEBUG_Update(game)
#define DEBUG_Render3D(game)
#define DEBUG_Render(game)
#define DEBUG_SetPerfStats(stats)
#define DEBUG_IsVisible() false

#endif /* DEBUG_ENABLED */
