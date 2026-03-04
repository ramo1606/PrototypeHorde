/*******************************************************************************************
*
*   level_manager.h — Level Manager (Level Transitions & Lifecycle)
*
*   LevelManager owns the active level pointer and orchestrates transitions between
*   levels using a three-state machine with pluggable visual effects. It ensures
*   clean handoffs: all actors are destroyed, the old level shuts down, game state
*   resets, and the new level initializes — all hidden behind a smooth visual overlay.
*
*   State Machine:
*       IDLE ──TransitionTo()──▶ FADING_OUT ──(progress reaches 1.0)──▶ [ApplySwap]
*                                                                           │
*       IDLE ◀──(progress reaches 0.0)── FADING_IN ◀───────────────────────┘
*
*   Architecture:
*       Game
*        └── LevelManager (embedded, not a pointer)
*               ├── activeLevel   → currently running Level*
*               ├── pendingLevel  → level to swap to (set during FADING_OUT)
*               ├── state         → IDLE / FADING_OUT / FADING_IN
*               ├── effectOut/In  → pluggable TransitionEffectFn
*               └── progress      → animation progress (0→1 out, 1→0 in)
*
*   Design Rationale:
*       Neither Madhav nor Knight implement a transition system — level changes are
*       instantaneous. We added this as an improvement for polish. Effects are simple
*       function pointers (TransitionEffectFn) that draw a fullscreen overlay given
*       a progress value [0,1]. This makes custom effects trivial to add.
*
*   Integration (called from game loop):
*       LEVEL_MGR_Update()  → before GAME_ProcessInput (drives state machine)
*       LEVEL_MGR_Render()  → after RENDERER_DrawFrame (draws overlay last)
*
*   Naming Convention:
*       Enums:   TRANSITION_*
*       API:     LEVEL_MGR_*
*
********************************************************************************************/
#pragma once

#include "level.h"
#include <stdbool.h>

typedef struct Game Game;
typedef struct LevelManager LevelManager;
typedef enum TransitionState TransitionState;

/* ── Transition State Machine ───────────────────────────────────────────── */

enum TransitionState
{
    TRANSITION_IDLE,                    /* No transition — normal gameplay               */
    TRANSITION_FADING_OUT,              /* Overlay covering screen (progress 0→1)        */
    TRANSITION_FADING_IN,               /* Overlay revealing new level (progress 1→0)    */
};

/* ── Transition Effects ─────────────────────────────────────────────────── */

/* A transition effect draws a fullscreen overlay based on progress [0,1].
 * progress=0 is fully transparent, progress=1 is fully opaque/covered.      */
typedef void (*TransitionEffectFn)(float progress);

void TRANSITION_Fade(float progress);       /* Alpha-blended black overlay   */
void TRANSITION_WipeLeft(float progress);   /* Left-to-right black curtain   */
void TRANSITION_WipeRight(float progress);  /* Right-to-left black curtain   */

/* ── Default Configuration ──────────────────────────────────────────────── */

#define TRANSITION_DEFAULT_DURATION   0.4f              /* Seconds per half-transition (out or in)        */
#define TRANSITION_DEFAULT_EFFECT_OUT TRANSITION_Fade    /* Default fade-out visual effect                 */
#define TRANSITION_DEFAULT_EFFECT_IN  NULL               /* NULL = reverse effectOut (progress goes 1→0)   */

/* ── Level Manager Struct ───────────────────────────────────────────────── */

struct LevelManager
{
    Level* activeLevel;                 /* Currently running level (NULL before first Init)  */
    Level* pendingLevel;                /* Level to swap to at peak of fade-out              */

    TransitionState state;              /* Current state machine state                       */
    TransitionEffectFn effectOut;       /* Visual effect for fade-out phase                  */
    TransitionEffectFn effectIn;        /* Visual effect for fade-in (NULL = reverse Out)    */
    float duration;                     /* Half-transition duration in seconds               */
    float progress;                     /* Animation progress: 0→1 (out), 1→0 (in)          */
};

/* ── Lifecycle ───────────────────────────────────────────────────────────── */

void LEVEL_MGR_Init(LevelManager* mgr, Game* game, Level* initialLevel);
void LEVEL_MGR_Shutdown(LevelManager* mgr, Game* game);
void LEVEL_MGR_Update(LevelManager* mgr, Game* game, float deltaTime);
void LEVEL_MGR_Render(const LevelManager* mgr);

/* ── Transition Control ─────────────────────────────────────────────────── */

void LEVEL_MGR_TransitionTo(LevelManager* mgr, Level* level,
    TransitionEffectFn effectOut,
    TransitionEffectFn effectIn,
    float duration);

/* ── Queries ─────────────────────────────────────────────────────────────── */

bool LEVEL_MGR_IsTransitioning(const LevelManager* mgr);
Level* LEVEL_MGR_GetActiveLevel(const LevelManager* mgr);
const char* LEVEL_MGR_GetStateName(const LevelManager* mgr);
float LEVEL_MGR_GetProgress(const LevelManager* mgr);
