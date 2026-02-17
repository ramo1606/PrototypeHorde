#pragma once

#include "level.h"
#include <stdbool.h>

typedef struct Game Game;

typedef enum
{
    TRANSITION_IDLE,
    TRANSITION_FADING_OUT,
    TRANSITION_FADING_IN,
} TransitionState;

typedef void (*TransitionEffectFn)(float progress);

void TRANSITION_Fade(float progress);
void TRANSITION_WipeLeft(float progress);
void TRANSITION_WipeRight(float progress);

#define TRANSITION_DEFAULT_DURATION 0.4f
#define TRANSITION_DEFAULT_EFFECT_OUT TRANSITION_Fade
#define TRANSITION_DEFAULT_EFFECT_IN NULL  /* NULL means "reverse effectOut" */

typedef struct
{
    /* Level tracking */
    Level* activeLevel;
    Level* pendingLevel;

    /* Transition */
    TransitionState    state;
    TransitionEffectFn effectOut;
    TransitionEffectFn effectIn;
    float              duration;
    float              progress;
} LevelManager;

void LEVEL_MGR_Init(LevelManager* mgr, Game* game, Level* initialLevel);
void LEVEL_MGR_Shutdown(LevelManager* mgr, Game* game);

void LEVEL_MGR_Update(LevelManager* mgr, Game* game, float deltaTime);
void LEVEL_MGR_Render(const LevelManager* mgr);

void LEVEL_MGR_TransitionTo(LevelManager* mgr, Level* level,
    TransitionEffectFn effectOut,
    TransitionEffectFn effectIn,
    float duration);

bool LEVEL_MGR_IsTransitioning(const LevelManager* mgr);
Level* LEVEL_MGR_GetActiveLevel(const LevelManager* mgr);
const char* LEVEL_MGR_GetStateName(const LevelManager* mgr);