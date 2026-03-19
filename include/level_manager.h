#pragma once

#include "level.h"
#include <stdbool.h>

typedef struct Game Game;

/* ── Transition State Machine ───────────────────────────────────────────── */

typedef enum TransitionState 
{
    TRANSITION_IDLE,
    TRANSITION_FADING_OUT,
    TRANSITION_FADING_IN,
} TransitionState;

/* A transition effect draws a fullscreen overlay based on progress [0,1]. */
typedef void (*TransitionEffectFn)(float progress);

/* Built-in effects */
void TRANSITION_Fade(float progress);
void TRANSITION_WipeLeft(float progress);
void TRANSITION_WipeRight(float progress);

/* ── Default Configuration ──────────────────────────────────────────────── */

#define TRANSITION_DEFAULT_DURATION 0.4f
#define TRANSITION_DEFAULT_EFFECT_OUT TRANSITION_Fade
#define TRANSITION_DEFAULT_EFFECT_IN NULL    /* NULL = reverse effectOut */

/* ── Level Manager Struct ───────────────────────────────────────────────── */

typedef struct LevelManager 
{
    Level* activeLevel;
    Level* pendingLevel;

    TransitionState state;
    TransitionEffectFn effectOut;
    TransitionEffectFn effectIn;
    float duration;
    float progress;       /* 0→1 (out), 1→0 (in) */
} LevelManager;

/* ── Lifecycle ───────────────────────────────────────────────────────────── */

void LEVEL_MGR_Init(LevelManager* mgr, Game* game, Level* initialLevel);
void LEVEL_MGR_Shutdown(LevelManager* mgr, Game* game);
void LEVEL_MGR_Update(LevelManager* mgr, Game* game, float dt);
void LEVEL_MGR_Render(const LevelManager* mgr);

/* ── Transition Control ─────────────────────────────────────────────────── */

void LEVEL_MGR_TransitionTo(LevelManager* mgr, Level* level,
    TransitionEffectFn effectOut,
    TransitionEffectFn effectIn,
    float duration);

/* Convenience: transition with default fade effect */
void LEVEL_MGR_SwitchTo(LevelManager* mgr, Level* level);

/* ── Queries ─────────────────────────────────────────────────────────────── */

bool LEVEL_MGR_IsTransitioning(const LevelManager* mgr);
Level* LEVEL_MGR_GetActiveLevel(const LevelManager* mgr);
const char* LEVEL_MGR_GetStateName(const LevelManager* mgr);
float LEVEL_MGR_GetProgress(const LevelManager* mgr);

/* ── Level Callbacks (delegated to active level) ─────────────────────────── */

void LEVEL_MGR_ProcessInput(LevelManager* mgr, Game* game);
void LEVEL_MGR_UpdateLevel(LevelManager* mgr, Game* game, float dt);
void LEVEL_MGR_RenderLevel(LevelManager* mgr, Game* game, float alpha);