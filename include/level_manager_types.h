#pragma once

#include "level.h"

typedef enum
{
    TRANSITION_IDLE,
    TRANSITION_FADING_OUT,
    TRANSITION_FADING_IN,
} TransitionState;

/* A transition effect draws a fullscreen overlay based on progress [0,1]. */
typedef void (*TransitionEffectFn)(float progress);

typedef struct LevelManager
{
    Level* activeLevel;
    Level* pendingLevel;

    TransitionState    state;
    TransitionEffectFn effectOut;
    TransitionEffectFn effectIn;
    float              duration;
    float              progress;       /* 0→1 (out), 1→0 (in) */
} LevelManager;
