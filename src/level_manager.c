#include "level_manager.h"
#include "raylib.h"
#include <assert.h>

/* ── Built-in Transition Effects ─────────────────────────────────────────── */

void TransitionFade(float progress)
{
    unsigned char alpha = (unsigned char)(progress * 255.0f);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
        (Color) { 0, 0, 0, alpha });
}

void TransitionWipeLeft(float progress)
{
    int w = (int)(progress * GetScreenWidth());
    DrawRectangle(0, 0, w, GetScreenHeight(), BLACK);
}

void TransitionWipeRight(float progress)
{
    int sw = GetScreenWidth();
    int w = (int)(progress * sw);
    DrawRectangle(sw - w, 0, w, GetScreenHeight(), BLACK);
}

/* ── Level Swap (internal) ───────────────────────────────────────────────── */
/* Called at the peak of fade-out (screen fully covered): old level's
 * Shutdown, then optional host onSwap (memory cleanup, etc.), then new
 * level's Init. The player never sees the teardown frame. */
static void applySwap(LevelManager* mgr)
{
    Level* newLevel = mgr->pendingLevel;
    mgr->pendingLevel = NULL;

    if (mgr->activeLevel && mgr->activeLevel->Shutdown)
        mgr->activeLevel->Shutdown(mgr->user);

    if (mgr->onSwap)
        mgr->onSwap(mgr->user);

    mgr->activeLevel = newLevel;
    if (newLevel && newLevel->Init)
        newLevel->Init(mgr->user);

    TraceLog(LOG_INFO, "LevelManager: Swapped to '%s'",
        newLevel ? newLevel->name : "(none)");
}

/* ── Lifecycle ───────────────────────────────────────────────────────────── */

void LevelManagerInit(LevelManager* mgr, void* user, Level* initialLevel)
{
    assert(mgr);

    mgr->activeLevel  = NULL;
    mgr->pendingLevel = NULL;
    mgr->state        = TRANSITION_IDLE;
    mgr->effectOut    = TRANSITION_DEFAULT_EFFECT_OUT;
    mgr->effectIn     = TRANSITION_DEFAULT_EFFECT_IN;
    mgr->duration     = TRANSITION_DEFAULT_DURATION;
    mgr->progress     = 0.0f;
    mgr->user         = user;
    mgr->onSwap       = NULL;

    /* Init the first level directly — no transition. */
    mgr->activeLevel = initialLevel;
    if (initialLevel && initialLevel->Init)
    {
        initialLevel->Init(user);
    }

    TraceLog(LOG_INFO, "LevelManager: Initialized with '%s'",
        initialLevel ? initialLevel->name : "(none)");
}

void LevelManagerShutdown(LevelManager* mgr)
{
    assert(mgr);

    if (mgr->activeLevel && mgr->activeLevel->Shutdown)
    {
        mgr->activeLevel->Shutdown(mgr->user);
    }

    mgr->activeLevel  = NULL;
    mgr->pendingLevel = NULL;
    mgr->state        = TRANSITION_IDLE;

    TraceLog(LOG_INFO, "LevelManager: Shutdown");
}

/* ── State Machine ───────────────────────────────────────────────────────── */

void LevelManagerUpdate(LevelManager* mgr, float dt)
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
            applySwap(mgr);
            mgr->state = TRANSITION_FADING_IN;
        }
        break;

    case TRANSITION_FADING_IN:
        mgr->progress -= speed * dt;
        if (mgr->progress <= 0.0f)
        {
            mgr->progress = 0.0f;
            mgr->state    = TRANSITION_IDLE;
            TraceLog(LOG_INFO, "LevelManager: Transition complete");
        }
        break;

    default: break;
    }
}

void LevelManagerRender(const LevelManager* mgr)
{
    assert(mgr);
    if (mgr->state == TRANSITION_IDLE) return;

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

/* ── Transition Control ──────────────────────────────────────────────────── */

void LevelManagerTransitionTo(LevelManager* mgr, Level* level,
                              TransitionEffectFn effectOut,
                              TransitionEffectFn effectIn,
                              float duration)
{
    assert(mgr);

    if (!level)
    {
        TraceLog(LOG_WARNING, "LevelManager: TransitionTo called with NULL level");
        return;
    }
    if (mgr->state != TRANSITION_IDLE)
    {
        TraceLog(LOG_WARNING, "LevelManager: Transition already in progress, ignoring");
        return;
    }
    if (level == mgr->activeLevel)
    {
        TraceLog(LOG_WARNING, "LevelManager: Already on '%s', ignoring", level->name);
        return;
    }

    mgr->pendingLevel = level;
    mgr->effectOut    = effectOut ? effectOut : TRANSITION_DEFAULT_EFFECT_OUT;
    mgr->effectIn     = effectIn;
    mgr->duration     = (duration > 0.0f) ? duration : TRANSITION_DEFAULT_DURATION;
    mgr->progress     = 0.0f;
    mgr->state        = TRANSITION_FADING_OUT;

    TraceLog(LOG_INFO, "LevelManager: Transitioning to '%s' (%.2fs)",
        level->name, mgr->duration);
}

void LevelManagerSwitchTo(LevelManager* mgr, Level* level)
{
    assert(mgr && level);
    LevelManagerTransitionTo(mgr, level, NULL, NULL, 0.0f);
}

/* ── Queries ─────────────────────────────────────────────────────────────── */

bool LevelManagerIsTransitioning(const LevelManager* mgr)
{
    assert(mgr);
    return mgr->state != TRANSITION_IDLE;
}

Level* LevelManagerGetActiveLevel(const LevelManager* mgr)
{
    assert(mgr);
    return mgr->activeLevel;
}

const char* LevelManagerGetStateName(const LevelManager* mgr)
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

float LevelManagerGetProgress(const LevelManager* mgr)
{
    assert(mgr);
    return mgr->progress;
}

/* ── Level Callbacks (delegated to active level) ─────────────────────────── */

void LevelManagerProcessInput(LevelManager* mgr)
{
    assert(mgr);
    if (mgr->activeLevel && mgr->activeLevel->ProcessInput)
        mgr->activeLevel->ProcessInput(mgr->user);
}

void LevelManagerUpdateLevel(LevelManager* mgr, float dt)
{
    assert(mgr);
    if (mgr->activeLevel && mgr->activeLevel->Update)
        mgr->activeLevel->Update(mgr->user, dt);
}

void LevelManagerRender3D(LevelManager* mgr, float alpha)
{
    assert(mgr);
    if (mgr->activeLevel && mgr->activeLevel->Render3D)
        mgr->activeLevel->Render3D(mgr->user, alpha);
}

void LevelManagerRenderHUD(LevelManager* mgr, float alpha)
{
    assert(mgr);
    if (mgr->activeLevel && mgr->activeLevel->RenderHUD)
        mgr->activeLevel->RenderHUD(mgr->user, alpha);
}
