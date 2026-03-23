#include "level_manager.h"
#include "game.h"
#include "memory.h"
#include "raylib.h"
#include <assert.h>

/* ═══════════════════════════════════════════════════════════════════════════
 *  Built-in Transition Effects
 * ═══════════════════════════════════════════════════════════════════════════ */

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
    int sw = GetScreenWidth();
    int w = (int)(progress * sw);
    DrawRectangle(sw - w, 0, w, GetScreenHeight(), BLACK);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Level Swap (Internal)
 *
 *  Called at the peak of fade-out (screen fully covered). The player
 *  never sees the teardown/rebuild.
 * ═══════════════════════════════════════════════════════════════════════════ */

static void ApplySwap(LevelManager* mgr, Game* game)
{
    Level* newLevel = mgr->pendingLevel;
    mgr->pendingLevel = NULL;

    /* Shutdown current level */
    if (mgr->activeLevel && mgr->activeLevel->Shutdown)
        mgr->activeLevel->Shutdown(game);

    /* Reset level arena — all level memory freed at once */
    ARENA_Reset(&game->level);

    /* Activate new level */
    mgr->activeLevel = newLevel;
    if (newLevel && newLevel->Init)
        newLevel->Init(game);

    TraceLog(LOG_INFO, "LEVEL_MGR: Swapped to '%s'",
        newLevel ? newLevel->name : "(none)");
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Lifecycle
 * ═══════════════════════════════════════════════════════════════════════════ */

void LEVEL_MGR_Init(LevelManager* mgr, Game* game, Level* initialLevel)
{
    assert(mgr && game);

    mgr->activeLevel = NULL;
    mgr->pendingLevel = NULL;
    mgr->state = TRANSITION_IDLE;
    mgr->effectOut = TRANSITION_DEFAULT_EFFECT_OUT;
    mgr->effectIn = TRANSITION_DEFAULT_EFFECT_IN;
    mgr->duration = TRANSITION_DEFAULT_DURATION;
    mgr->progress = 0.0f;

    /* Init the first level directly — no transition */
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
    assert(mgr && game);

    if (mgr->activeLevel && mgr->activeLevel->Shutdown) 
    {
        mgr->activeLevel->Shutdown(game);
    }

    mgr->activeLevel = NULL;
    mgr->pendingLevel = NULL;
    mgr->state = TRANSITION_IDLE;

    TraceLog(LOG_INFO, "LEVEL_MGR: Shutdown");
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  State Machine
 * ═══════════════════════════════════════════════════════════════════════════ */

void LEVEL_MGR_Update(LevelManager* mgr, Game* game, float dt)
{
    assert(mgr);
    if (mgr->state == TRANSITION_IDLE) return;

    float speed = (mgr->duration > 0.0f) ? (1.0f / mgr->duration) : 100.0f;

    switch (mgr->state)
    {
    case TRANSITION_FADING_OUT:
        mgr->progress += speed * dt;
        if (mgr->progress >= 1.0f) 
        {
            mgr->progress = 1.0f;
            ApplySwap(mgr, game);
            mgr->state = TRANSITION_FADING_IN;
        }
        break;

    case TRANSITION_FADING_IN:
        mgr->progress -= speed * dt;
        if (mgr->progress <= 0.0f)
        {
            mgr->progress = 0.0f;
            mgr->state = TRANSITION_IDLE;
            TraceLog(LOG_INFO, "LEVEL_MGR: Transition complete");
        }
        break;

    default: break;
    }
}

void LEVEL_MGR_Render(const LevelManager* mgr)
{
    assert(mgr);
    if (mgr->state == TRANSITION_IDLE)
    {
        return;
    }

    TransitionEffectFn effect;
    if (mgr->state == TRANSITION_FADING_IN && mgr->effectIn)
    {
        effect = mgr->effectIn;
    }
    else
    {
        effect = mgr->effectOut;
    }

    if (effect)
    {
        effect(mgr->progress);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Transition Control
 * ═══════════════════════════════════════════════════════════════════════════ */

void LEVEL_MGR_TransitionTo(LevelManager* mgr, Level* level,
    TransitionEffectFn effectOut,
    TransitionEffectFn effectIn,
    float duration)
{
    assert(mgr);

    if (!level) 
    {
        TraceLog(LOG_WARNING, "LEVEL_MGR: TransitionTo called with NULL level");
        return;
    }
    if (mgr->state != TRANSITION_IDLE) 
    {
        TraceLog(LOG_WARNING, "LEVEL_MGR: Transition already in progress, ignoring");
        return;
    }
    if (level == mgr->activeLevel) 
    {
        TraceLog(LOG_WARNING, "LEVEL_MGR: Already on '%s', ignoring", level->name);
        return;
    }

    mgr->pendingLevel = level;
    mgr->effectOut = effectOut ? effectOut : TRANSITION_DEFAULT_EFFECT_OUT;
    mgr->effectIn = effectIn;
    mgr->duration = (duration > 0.0f) ? duration : TRANSITION_DEFAULT_DURATION;
    mgr->progress = 0.0f;
    mgr->state = TRANSITION_FADING_OUT;

    TraceLog(LOG_INFO, "LEVEL_MGR: Transitioning to '%s' (%.2fs)",
        level->name, mgr->duration);
}

void LEVEL_MGR_SwitchTo(LevelManager* mgr, Level* level)
{
	assert(mgr && level);
    LEVEL_MGR_TransitionTo(mgr, level, NULL, NULL, 0.0f);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Queries
 * ═══════════════════════════════════════════════════════════════════════════ */

bool LEVEL_MGR_IsTransitioning(const LevelManager* mgr)
{
    assert(mgr);
    return mgr->state != TRANSITION_IDLE;
}

Level* LEVEL_MGR_GetActiveLevel(const LevelManager* mgr)
{
    assert(mgr);
    return mgr->activeLevel;
}

const char* LEVEL_MGR_GetStateName(const LevelManager* mgr)
{
	assert(mgr);
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
	assert(mgr);
    return mgr->progress;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Level Callbacks (delegated to active level)
 * ═══════════════════════════════════════════════════════════════════════════ */

void LEVEL_MGR_ProcessInput(LevelManager* mgr, Game* game)
{
	assert(mgr && game);

    if (mgr->activeLevel && mgr->activeLevel->ProcessInput)
    {
        mgr->activeLevel->ProcessInput(game);
    }
}

void LEVEL_MGR_UpdateLevel(LevelManager* mgr, Game* game, float dt)
{
	assert(mgr && game);

    if (mgr->activeLevel && mgr->activeLevel->Update)
    {
        mgr->activeLevel->Update(game, dt);
    }
}

void LEVEL_MGR_Render3D(LevelManager* mgr, Game* game, float alpha)
{
    assert(mgr && game);

    if (mgr->activeLevel && mgr->activeLevel->Render3D)
    {
        mgr->activeLevel->Render3D(game, alpha);
    }
}

void LEVEL_MGR_RenderHUD(LevelManager* mgr, Game* game, float alpha)
{
    assert(mgr && game);

    if (mgr->activeLevel && mgr->activeLevel->RenderHUD)
    {
        mgr->activeLevel->RenderHUD(game, alpha);
    }
}