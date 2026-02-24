#include "level_manager.h"
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

static void LEVEL_MGR_ApplySwap(LevelManager* mgr, Game* game)
{
    assert(mgr != NULL);
    assert(game != NULL);

    Level* newLevel = mgr->pendingLevel;
    mgr->pendingLevel = NULL;

    /* Shutdown current */
    GAME_RemoveAllActors(game);
    if (mgr->activeLevel && mgr->activeLevel->Shutdown)
    {
        mgr->activeLevel->Shutdown(game);
    }

    /* Reset game state for the new level */
    game->state = GAME_STATE_GAMEPLAY;
    game->accumulator = 0.0f;
    game->actorsCreated = 0;

    /* Reset camera to default — levels with CameraTPS override in Init */
    //TODO: Resetting the camera like this is a bit hacky, but it works for now. Maybe add a RENDERER_ResetCamera or similar?
    RENDERER_SetCamera(&game->renderer, (Camera3D){
        .position   = (Vector3){ 15.0f, 12.0f, 15.0f },
        .target     = (Vector3){ 0.0f, 0.0f, 0.0f },
        .up         = (Vector3){ 0.0f, 1.0f, 0.0f },
        .fovy       = 45.0f,
        .projection = CAMERA_PERSPECTIVE,
    });
    RENDERER_SetClearColor(&game->renderer, (Color){ 20, 20, 40, 255 });

    /* Init new */
    mgr->activeLevel = newLevel;
    if (newLevel && newLevel->Init)
    {
        newLevel->Init(game);
    }

    TraceLog(LOG_INFO, "LEVEL_MGR: Swapped to '%s'",
        newLevel ? newLevel->name : "(none)");
}

void LEVEL_MGR_Init(LevelManager* mgr, Game* game, Level* initialLevel)
{
    assert(mgr != NULL);
    assert(game != NULL);

    mgr->activeLevel = NULL;
    mgr->pendingLevel = NULL;
    mgr->state = TRANSITION_IDLE;
    mgr->effectOut = TRANSITION_DEFAULT_EFFECT_OUT;
    mgr->effectIn = TRANSITION_DEFAULT_EFFECT_IN;
    mgr->duration = TRANSITION_DEFAULT_DURATION;
    mgr->progress = 0.0f;

    /* Init the first level directly � no transition */
    mgr->activeLevel = initialLevel;
    if (initialLevel && initialLevel->Init)
    {
        initialLevel->Init(game);
    }

    TraceLog(LOG_INFO, "LEVEL_MGR: Initialized with '%s'",
        initialLevel ? initialLevel->name : "(none)");
}

void LEVEL_MGR_Shutdown(LevelManager* mgr, Game* game)
{
    assert(mgr != NULL);
    assert(game != NULL);

    GAME_RemoveAllActors(game);

    if (mgr->activeLevel && mgr->activeLevel->Shutdown)
    {
        mgr->activeLevel->Shutdown(game);
    }

    mgr->activeLevel = NULL;
    mgr->pendingLevel = NULL;
    mgr->state = TRANSITION_IDLE;

    TraceLog(LOG_INFO, "LEVEL_MGR: Shutdown");
}

void LEVEL_MGR_Update(LevelManager* mgr, Game* game, float deltaTime)
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

                /* Screen fully covered � do the swap */
                LEVEL_MGR_ApplySwap(mgr, game);

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

                TraceLog(LOG_INFO, "LEVEL_MGR: Transition complete");
            }
        } break;

        default: break;
    }
}

void LEVEL_MGR_Render(const LevelManager* mgr)
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

void LEVEL_MGR_TransitionTo(LevelManager* mgr, Level* level,
    TransitionEffectFn effectOut,
    TransitionEffectFn effectIn,
    float duration)
{
    assert(mgr != NULL);

    if (!level)
    {
        TraceLog(LOG_WARNING, "LEVEL_MGR: TransitionTo called with NULL level");
        return;
    }

    /* Ignore if already transitioning */
    if (mgr->state != TRANSITION_IDLE)
    {
        TraceLog(LOG_WARNING, "LEVEL_MGR: Transition already in progress, ignoring");
        return;
    }

    /* Ignore if same level */
    if (level == mgr->activeLevel)
    {
        TraceLog(LOG_WARNING, "LEVEL_MGR: Already on '%s', ignoring", level->name);
        return;
    }

    mgr->pendingLevel = level;
    mgr->effectOut = effectOut ? effectOut : TRANSITION_DEFAULT_EFFECT_OUT;
    mgr->effectIn = effectIn;  /* NULL is valid � means "reverse effectOut" */
    mgr->duration = (duration > 0.0f) ? duration : TRANSITION_DEFAULT_DURATION;
    mgr->progress = 0.0f;
    mgr->state = TRANSITION_FADING_OUT;

    TraceLog(LOG_INFO, "LEVEL_MGR: Transitioning to '%s' (%.2fs)",
        level->name, mgr->duration);
}

bool LEVEL_MGR_IsTransitioning(const LevelManager* mgr)
{
    assert(mgr != NULL);
    return mgr->state != TRANSITION_IDLE;
}

Level* LEVEL_MGR_GetActiveLevel(const LevelManager* mgr)
{
    assert(mgr != NULL);
    return mgr->activeLevel;
}

const char* LEVEL_MGR_GetStateName(const LevelManager* mgr)
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

float LEVEL_MGR_GetProgress(const LevelManager* mgr)
{
    assert(mgr != NULL);
	return mgr->progress;
}
