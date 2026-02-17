#include "scene_manager.h"
#include "game.h"
#include "raylib.h"
#include <assert.h>

void TRANSITION_Fade(float progress)
{
    unsigned char alpha = (unsigned char)(progress * 255.0f);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
        (Color) {
        0, 0, 0, alpha
    });
}

void TRANSITION_WipeLeft(float progress)
{
    int w = (int)(progress * GetScreenWidth());
    DrawRectangle(0, 0, w, GetScreenHeight(), BLACK);
}

void TRANSITION_WipeRight(float progress)
{
    int screenW = GetScreenWidth();
    int w = (int)(progress * screenW);
    DrawRectangle(screenW - w, 0, w, GetScreenHeight(), BLACK);
}

static void SCENE_MGR_ApplySwap(SceneManager* mgr, Game* game)
{
    assert(mgr != NULL);
    assert(game != NULL);

    Scene* newScene = mgr->pendingScene;
    mgr->pendingScene = NULL;

    /* Shutdown current */
    GAME_RemoveAllActors(game);
    if (mgr->activeScene && mgr->activeScene->Shutdown)
    {
        mgr->activeScene->Shutdown(game);
    }

    /* Reset game state for the new scene */
    game->state = GAME_STATE_GAMEPLAY;
    game->accumulator = 0.0f;
    game->actorsCreated = 0;

    /* Init new */
    mgr->activeScene = newScene;
    if (newScene && newScene->Init)
    {
        newScene->Init(game);
    }

    TraceLog(LOG_INFO, "SCENE_MGR: Swapped to '%s'",
        newScene ? newScene->name : "(none)");
}

void SCENE_MGR_Init(SceneManager* mgr, Game* game, Scene* initialScene)
{
    assert(mgr != NULL);
    assert(game != NULL);

    mgr->activeScene = NULL;
    mgr->pendingScene = NULL;
    mgr->state = TRANSITION_IDLE;
    mgr->effectOut = TRANSITION_DEFAULT_EFFECT_OUT;
    mgr->effectIn = TRANSITION_DEFAULT_EFFECT_IN;
    mgr->duration = TRANSITION_DEFAULT_DURATION;
    mgr->progress = 0.0f;

    /* Init the first scene directly — no transition */
    mgr->activeScene = initialScene;
    if (initialScene && initialScene->Init)
    {
        initialScene->Init(game);
    }

    TraceLog(LOG_INFO, "SCENE_MGR: Initialized with '%s'",
        initialScene ? initialScene->name : "(none)");
}

void SCENE_MGR_Shutdown(SceneManager* mgr, Game* game)
{
    assert(mgr != NULL);
    assert(game != NULL);

    GAME_RemoveAllActors(game);

    if (mgr->activeScene && mgr->activeScene->Shutdown)
    {
        mgr->activeScene->Shutdown(game);
    }

    mgr->activeScene = NULL;
    mgr->pendingScene = NULL;
    mgr->state = TRANSITION_IDLE;

    TraceLog(LOG_INFO, "SCENE_MGR: Shutdown");
}

void SCENE_MGR_Update(SceneManager* mgr, Game* game, float deltaTime)
{
    assert(mgr != NULL);

    if (mgr->state == TRANSITION_IDLE) return;

    float speed = (mgr->duration > 0.0f) ? (1.0f / mgr->duration) : 100.0f;

    switch (mgr->state)
    {
        case TRANSITION_FADING_OUT:
        {
            mgr->progress += speed * deltaTime;
            if (mgr->progress >= 1.0f)
            {
                mgr->progress = 1.0f;

                /* Screen fully covered — do the swap */
                SCENE_MGR_ApplySwap(mgr, game);

                mgr->state = TRANSITION_FADING_IN;
            }
        } break;

        case TRANSITION_FADING_IN:
        {
            mgr->progress -= speed * deltaTime;
            if (mgr->progress <= 0.0f)
            {
                mgr->progress = 0.0f;
                mgr->state = TRANSITION_IDLE;

                TraceLog(LOG_INFO, "SCENE_MGR: Transition complete");
            }
        } break;

        default: break;
    }
}

void SCENE_MGR_Render(const SceneManager* mgr)
{
    assert(mgr != NULL);

    if (mgr->state == TRANSITION_IDLE) return;

    /* Pick effect: effectIn for FADING_IN, fallback to effectOut (reverses naturally) */
    TransitionEffectFn effect;
    if (mgr->state == TRANSITION_FADING_IN && mgr->effectIn)
    {
        effect = mgr->effectIn;
    }
    else
    {
        effect = mgr->effectOut;
    }

    if (effect) effect(mgr->progress);
}

void SCENE_MGR_TransitionTo(SceneManager* mgr, Scene* scene,
    TransitionEffectFn effectOut,
    TransitionEffectFn effectIn,
    float duration)
{
    assert(mgr != NULL);

    if (!scene)
    {
        TraceLog(LOG_WARNING, "SCENE_MGR: TransitionTo called with NULL scene");
        return;
    }

    /* Ignore if already transitioning */
    if (mgr->state != TRANSITION_IDLE)
    {
        TraceLog(LOG_WARNING, "SCENE_MGR: Transition already in progress, ignoring");
        return;
    }

    /* Ignore if same scene */
    if (scene == mgr->activeScene)
    {
        TraceLog(LOG_WARNING, "SCENE_MGR: Already on '%s', ignoring", scene->name);
        return;
    }

    mgr->pendingScene = scene;
    mgr->effectOut = effectOut ? effectOut : TRANSITION_DEFAULT_EFFECT_OUT;
    mgr->effectIn = effectIn;  /* NULL is valid — means "reverse effectOut" */
    mgr->duration = (duration > 0.0f) ? duration : TRANSITION_DEFAULT_DURATION;
    mgr->progress = 0.0f;
    mgr->state = TRANSITION_FADING_OUT;

    TraceLog(LOG_INFO, "SCENE_MGR: Transitioning to '%s' (%.2fs)",
        scene->name, mgr->duration);
}

bool SCENE_MGR_IsTransitioning(const SceneManager* mgr)
{
    assert(mgr != NULL);
    return mgr->state != TRANSITION_IDLE;
}

Scene* SCENE_MGR_GetActiveScene(const SceneManager* mgr)
{
    assert(mgr != NULL);
    return mgr->activeScene;
}

const char* SCENE_MGR_GetStateName(const SceneManager* mgr)
{
    assert(mgr != NULL);
    switch (mgr->state)
    {
        case TRANSITION_IDLE:       return "IDLE";
        case TRANSITION_FADING_OUT: return "FADING_OUT";
        case TRANSITION_FADING_IN:  return "FADING_IN";
        default:                    return "UNKNOWN";
    }
}
