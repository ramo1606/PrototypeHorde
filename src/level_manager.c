/*******************************************************************************************
*
*   level_manager.c — Level Manager Implementation
*
********************************************************************************************/
#include "level_manager.h"
#include "game.h"
#include "raylib.h"
#include <assert.h>

/* ═══════════════════════════════════════════════════════════════════════════
 *  Built-in Transition Effects
 *
 *  Each effect takes a progress value [0,1] and draws a fullscreen overlay.
 *  At progress=0 the screen is fully visible; at progress=1 fully covered.
 *  During FADING_IN, progress goes 1→0, so the effect naturally reverses.
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Fade: alpha-blended black rectangle covers the entire screen. */
void TRANSITION_Fade(float progress)
{
    unsigned char alpha = (unsigned char)(progress * 255.0f);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
        (Color) {
        0, 0, 0, alpha
    });
}

/* Wipe left: solid black expands from the left edge rightward. */
void TRANSITION_WipeLeft(float progress)
{
    int w = (int)(progress * GetScreenWidth());
    DrawRectangle(0, 0, w, GetScreenHeight(), BLACK);
}

/* Wipe right: solid black expands from the right edge leftward. */
void TRANSITION_WipeRight(float progress)
{
    int screenW = GetScreenWidth();
    int w = (int)(progress * screenW);
    DrawRectangle(screenW - w, 0, w, GetScreenHeight(), BLACK);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Level Swap Logic (Internal)
 * ═══════════════════════════════════════════════════════════════════════════ */

/*------------------------------------------------------------------------------------
 * LEVEL_MGR_ApplySwap (static)
 *
 *   Performs the actual level swap at the peak of a fade-out (screen fully covered).
 *   This is the only moment where both levels are "neither here nor there" — the
 *   player can't see the transition, so we can safely tear down and rebuild.
 *
 *   Swap sequence:
 *     1. Destroy all actors (components auto-unregister from Renderer/PhysWorld)
 *     2. Call old level's Shutdown (level-specific cleanup)
 *     3. Reset game state (gameplay mode, zero accumulator, reset camera)
 *     4. Set new level as active
 *     5. Call new level's Init (spawns fresh actors)
 *
 *   The camera reset to a default isometric view ensures levels that don't use
 *   CameraTPS still have a reasonable view. Levels with CameraTPS override it
 *   during their Init callback.
 *------------------------------------------------------------------------------------*/
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

/* ═══════════════════════════════════════════════════════════════════════════
 *  Lifecycle
 * ═══════════════════════════════════════════════════════════════════════════ */

/*------------------------------------------------------------------------------------
 * LEVEL_MGR_Init
 *
 *   Sets up the level manager and loads the first level directly (no transition).
 *   The initial level bypasses the state machine because there's nothing to fade
 *   from — we go straight to Init without any visual effect.
 *------------------------------------------------------------------------------------*/
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

    /* Init the first level directly — no transition */
    mgr->activeLevel = initialLevel;
    if (initialLevel && initialLevel->Init)
    {
        initialLevel->Init(game);
    }

    TraceLog(LOG_INFO, "LEVEL_MGR: Initialized with '%s'",
        initialLevel ? initialLevel->name : "(none)");
}

/* Teardown: destroy all actors, call active level's Shutdown, clear state. */
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

/* ═══════════════════════════════════════════════════════════════════════════
 *  State Machine
 * ═══════════════════════════════════════════════════════════════════════════ */

/*------------------------------------------------------------------------------------
 * LEVEL_MGR_Update
 *
 *   Drives the transition state machine each frame. Progress ramps from 0 to 1
 *   during FADING_OUT, then from 1 back to 0 during FADING_IN. The ramp speed
 *   is derived from the configured duration: speed = 1.0 / duration.
 *
 *   At the FADING_OUT → FADING_IN boundary (progress hits 1.0), ApplySwap runs
 *   while the screen is fully covered. This guarantees the player never sees the
 *   old level being torn down or the new level being built.
 *
 *   The state machine is intentionally simple — no LOADING state, no async —
 *   because level Init is synchronous and fast (just spawning actors from pools).
 *------------------------------------------------------------------------------------*/
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

                /* Screen fully covered — do the swap */
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

/*------------------------------------------------------------------------------------
 * LEVEL_MGR_Render
 *
 *   Draws the transition overlay on top of everything else. Called last in the
 *   render pipeline (after HUD, debug, pause) so the transition covers all content.
 *
 *   Effect selection: uses effectIn during FADING_IN if one was specified.
 *   If effectIn is NULL, falls back to effectOut which reverses naturally
 *   because progress goes 1→0 (the overlay fades away using the same function).
 *------------------------------------------------------------------------------------*/
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

/* ═══════════════════════════════════════════════════════════════════════════
 *  Transition Control
 * ═══════════════════════════════════════════════════════════════════════════ */

/*------------------------------------------------------------------------------------
 * LEVEL_MGR_TransitionTo
 *
 *   Initiates a level transition with three safety guards:
 *     1. NULL level — logs a warning and returns (can't transition to nothing)
 *     2. Already transitioning — ignores to prevent state machine corruption
 *     3. Same level — ignores to avoid pointless destroy-and-rebuild
 *
 *   Sets up the transition parameters (effects, duration) and kicks the state
 *   machine from IDLE to FADING_OUT. The actual swap happens asynchronously
 *   in LEVEL_MGR_Update when progress reaches 1.0.
 *
 *   effectOut defaults to TRANSITION_Fade if NULL. effectIn NULL is valid and
 *   means "reverse the effectOut" (progress goes 1→0 using the same function).
 *------------------------------------------------------------------------------------*/
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
    mgr->effectIn = effectIn;  /* NULL is valid — means "reverse effectOut" */
    mgr->duration = (duration > 0.0f) ? duration : TRANSITION_DEFAULT_DURATION;
    mgr->progress = 0.0f;
    mgr->state = TRANSITION_FADING_OUT;

    TraceLog(LOG_INFO, "LEVEL_MGR: Transitioning to '%s' (%.2fs)",
        level->name, mgr->duration);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Queries
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Returns true if a transition is in progress (FADING_OUT or FADING_IN). */
bool LEVEL_MGR_IsTransitioning(const LevelManager* mgr)
{
    assert(mgr != NULL);
    return mgr->state != TRANSITION_IDLE;
}

/* Returns the currently active level (may be NULL before first Init). */
Level* LEVEL_MGR_GetActiveLevel(const LevelManager* mgr)
{
    assert(mgr != NULL);
    return mgr->activeLevel;
}

/* Returns a human-readable name for the current transition state (for debug). */
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

/* Returns the raw transition progress value [0,1] (for debug overlay). */
float LEVEL_MGR_GetProgress(const LevelManager* mgr)
{
    assert(mgr != NULL);
    return mgr->progress;
}
