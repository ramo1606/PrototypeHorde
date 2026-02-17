#pragma once

#include "scene.h"
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
    /* Scene tracking */
    Scene* activeScene;
    Scene* pendingScene;

    /* Transition */
    TransitionState    state;
    TransitionEffectFn effectOut;
    TransitionEffectFn effectIn;
    float              duration;
    float              progress;
} SceneManager;

void SCENE_MGR_Init(SceneManager* mgr, Game* game, Scene* initialScene);
void SCENE_MGR_Shutdown(SceneManager* mgr, Game* game);

void SCENE_MGR_Update(SceneManager* mgr, Game* game, float deltaTime);
void SCENE_MGR_Render(const SceneManager* mgr);

void SCENE_MGR_TransitionTo(SceneManager* mgr, Scene* scene,
    TransitionEffectFn effectOut,
    TransitionEffectFn effectIn,
    float duration);

bool SCENE_MGR_IsTransitioning(const SceneManager* mgr);
Scene* SCENE_MGR_GetActiveScene(const SceneManager* mgr);
const char* SCENE_MGR_GetStateName(const SceneManager* mgr);