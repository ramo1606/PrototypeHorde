/*******************************************************************************************
*
*   game.c — Game Core Implementation
*
********************************************************************************************/
#include "actor.h"
#include "component.h"
#include "debug.h"
#include "mesh_component.h"
#include "level.h"
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ── Forward Declarations (internal helpers) ─────────────────────────────── */
static void GAME_ProcessInput(Game* game);
static void GAME_FixedUpdate(Game* game, float deltaTime);

static void GAME_RemoveActiveActorByIndex(Game* game, int idx);
static void GAME_RemovePendingActorByIndex(Game* game, int idx);

/*------------------------------------------------------------------------------------
 * GAME_Init
 *
 *   Bootstraps the entire engine. Initialization order matters:
 *
 *     1. Memory system first — everything else allocates from it.
 *     2. Window (Raylib) — needed before renderer queries GPU capabilities.
 *     3. Renderer — loads default shaders, sets up framebuffers.
 *     4. Physics world — clears spatial data structures.
 *     5. Debug overlay — depends on window being ready.
 *     6. Level manager last — may spawn actors that use all of the above.
 *
 *   The memset to zero ensures all pointers, counts, and flags start clean.
 *   This is safe because 0 is a valid initial value for every Game field
 *   (state=GAMEPLAY is 0, all counts are 0, all pointers NULL).
 *------------------------------------------------------------------------------------*/
bool GAME_Init(Game* game, Level* initialLevel)
{
    if (!game)
    {
        TraceLog(LOG_ERROR, "GAME_Init: game pointer is NULL");
        return false;
    }

    memset(game, 0, sizeof(Game));
    game->state = GAME_STATE_GAMEPLAY;
	game->accumulator = 0.0f;
	game->updateCount = 0;

    /* ── 1. Memory pools ──────────────────────────────────────────────── */
    MEMORY_Init(&game->memory);
    
    /* ── 2. Window ────────────────────────────────────────────────────── */
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, GAME_TITLE);
    if (!IsWindowReady())
    {
        TraceLog(LOG_ERROR, "Failed to initialize window");
        return false;
    }

    SetWindowState(FLAG_VSYNC_HINT);
	SetTargetFPS(RENDER_FPS);

    /* ── 3-5. Subsystems ──────────────────────────────────────────────── */
    RENDERER_Init(&game->renderer);
    PHYS_WORLD_Init(&game->physWorld);
    DEBUG_Init();

    /* ── 6. Level manager (may spawn actors) ──────────────────────────── */
    LEVEL_MGR_Init(&game->levelMgr, game, initialLevel);

    TraceLog(LOG_INFO, "Game initialized - Updates: %dHz, Rendering: %dFPS",
        UPDATE_RATE, GetFPS());

	return true;
}

/*------------------------------------------------------------------------------------
 * GAME_Shutdown
 *
 *   Teardown in reverse initialization order. Level manager destroys all actors
 *   first so that components can unregister from physics/renderer before those
 *   systems are torn down. Memory system shuts down last since everything else
 *   was allocated from it.
 *
 *   CloseWindow() is called between renderer and memory shutdown because Raylib
 *   owns GPU resources that the renderer references.
 *------------------------------------------------------------------------------------*/
void GAME_Shutdown(Game* game)
{
    if (!game)
    {
        TraceLog(LOG_ERROR, "GAME_Shutdown: game pointer is NULL");
        return;
    }

    LEVEL_MGR_Shutdown(&game->levelMgr, game);

    DEBUG_Shutdown();
    PHYS_WORLD_Shutdown(&game->physWorld);
    RENDERER_Shutdown(&game->renderer);

    CloseWindow();

    MEMORY_Shutdown(&game->memory);
    
    TraceLog(LOG_INFO, "Game shutdown - Time: %.2fs",
        GetTime());
}

/*------------------------------------------------------------------------------------
 * GAME_Run — Main Game Loop
 *
 *   Implements Glenn Fiedler's "Fix Your Timestep!" pattern, which decouples
 *   physics simulation from rendering framerate.
 *
 *   Reference: https://gafferongames.com/post/fix_your_timestep/
 *
 *   The problem: if physics steps use variable delta time, simulation becomes
 *   non-deterministic and unstable (tunneling, jittery collisions). If physics
 *   uses a fixed step locked to framerate, slow machines see slow-motion.
 *
 *   The solution: accumulate real elapsed time and drain it in fixed-size chunks.
 *   Any leftover time (< FIXED_TIMESTEP) is used as an interpolation alpha so
 *   rendering appears smooth even though physics runs at a different rate.
 *
 *   Frame structure:
 *
 *       ┌─ frameTime ──────────────────────────────────────────┐
 *       │                                                      │
 *       │  Level/Debug update → Input → State-based visuals    │
 *       │                                                      │
 *       │  Save previous transforms (for interpolation)        │
 *       │                                                      │
 *       │  ┌── FixedUpdate ──┐  ┌── FixedUpdate ──┐           │
 *       │  │  tick at 16.67ms│  │  tick at 16.67ms│  leftover │
 *       │  └─────────────────┘  └─────────────────┘    ◄──►   │
 *       │                                              alpha   │
 *       │  Interpolate transforms (alpha blend)                │
 *       │  Render                                              │
 *       │  Restore transforms (so next tick is from real state)│
 *       └──────────────────────────────────────────────────────┘
 *
 *   MAX_DELTA_TIME prevents the "spiral of death" — if a frame takes too long
 *   (breakpoint, OS stall), we cap it rather than trying to simulate 10+ ticks
 *   in one frame, which would make the next frame even slower.
 *------------------------------------------------------------------------------------*/
void GAME_Run(Game* game)
{
    if (!game)
    {
        TraceLog(LOG_ERROR, "GAME_Run: game pointer is NULL");
        return;
    }

    while (!WindowShouldClose() && game->state != GAME_STATE_QUIT)
    {
        /* ── Frame timing with spiral-of-death protection ─────────── */
        float frameTime = GetFrameTime();
        if (frameTime > MAX_DELTA_TIME)
        {
            frameTime = MAX_DELTA_TIME;
        }

        /* ── Per-frame updates (not tied to fixed timestep) ───────── */
        LEVEL_MGR_Update(&game->levelMgr, game, frameTime);
		DEBUG_Update(game);
        GAME_ProcessInput(game);

        /* ── Visual feedback for game state ───────────────────────── */
        switch (game->state)
        {
        case GAME_STATE_GAMEPLAY:
            RENDERER_SetClearColor(&game->renderer, (Color) { 20, 20, 40, 255 });
            break;
        case GAME_STATE_PAUSED:
            RENDERER_SetClearColor(&game->renderer, (Color) { 40, 20, 20, 255 });
            break;
        default:
            RENDERER_SetClearColor(&game->renderer, BLACK);
            break;
        }

        /* ── Fixed-timestep accumulator loop ──────────────────────── */
        game->accumulator += frameTime;
		game->updateCount = 0;

        /*
         * Save previous transforms BEFORE any physics ticks.
         * We need the "state before this frame" for interpolation.
         */
        for (int i = 0; i < game->actorCount; i++)
        {
            SCENE_COMPONENT_SavePrevState(&game->actors[i]->root);
        }

        /*
         * Drain the accumulator in fixed-size steps.
         * Each step is deterministic — always the same deltaTime.
         */
        while (game->accumulator >= FIXED_TIMESTEP)
        {
            GAME_FixedUpdate(game, FIXED_TIMESTEP);
            game->accumulator -= FIXED_TIMESTEP;
			game->updateCount++;
        }

        /* ── Interpolation for smooth rendering ───────────────────── */
        /*
         * alpha ∈ [0, 1) represents how far into the next tick we are.
         * At alpha=0 we render the last completed physics state.
         * At alpha=0.99 we render almost at the next tick's state.
         *
         * renderPos = prevPos + (currentPos - prevPos) * alpha
         *
         * This eliminates the visual jitter that comes from physics
         * and rendering running at different frequencies.
         */
        float alpha = game->accumulator / FIXED_TIMESTEP;
        for (int i = 0; i < game->actorCount; i++)
        {
            SCENE_COMPONENT_InterpolateForRender(&game->actors[i]->root, alpha);
        }

        /* ── Render the interpolated scene ────────────────────────── */
        RENDERER_DrawFrame(&game->renderer, game);

        /*
         * Restore transforms to their true physics state so the next
         * frame's fixed-update starts from correct (non-interpolated) values.
         */
        for (int i = 0; i < game->actorCount; i++)
        {
            SCENE_COMPONENT_RestoreFromInterpolation(&game->actors[i]->root);
        }
    }
}

/*------------------------------------------------------------------------------------
 * GAME_ProcessInput (internal)
 *
 *   Input handling runs once per frame (not per fixed-step) to ensure responsive
 *   controls. Processes in priority order:
 *
 *     1. Global keys (pause, quit) — always active
 *     2. Level-specific input — only if not transitioning
 *     3. Actor input — only during gameplay state
 *
 *   The level transition guard prevents input during fade-out/fade-in,
 *   avoiding state corruption while actors are being swapped.
 *------------------------------------------------------------------------------------*/
void GAME_ProcessInput(Game* game)
{
	assert(game != NULL);

    /* ── Global hotkeys (always active) ───────────────────────────── */
    if (IsKeyPressed(KEY_P)) 
    {
        if (game->state == GAME_STATE_GAMEPLAY)
            game->state = GAME_STATE_PAUSED;
        else if (game->state == GAME_STATE_PAUSED)
            game->state = GAME_STATE_GAMEPLAY;
    }

    if (IsKeyPressed(KEY_ESCAPE)) 
    {
        game->state = GAME_STATE_QUIT;
    }

    /* ── Level-specific and actor input (guarded) ─────────────────── */
    if (LEVEL_MGR_IsTransitioning(&game->levelMgr)) return;

    Level* active = LEVEL_MGR_GetActiveLevel(&game->levelMgr);
    if (active && active->ProcessInput)
    {
        active->ProcessInput(game);
    }

    if (game->state == GAME_STATE_GAMEPLAY) 
    {
        for (int i = 0; i < game->actorCount; i++) 
        {
            ACTOR_ProcessInput(game->actors[i]);
        }
    }
}

/*------------------------------------------------------------------------------------
 * GAME_FixedUpdate (internal)
 *
 *   One deterministic simulation tick. Executes the three-phase actor update
 *   pattern commonly used in game engines (Unreal, Unity use similar approaches):
 *
 *   Phase 1 — Update active actors
 *     Sets updatingActors=true as a guard. Any GAME_AddActor calls during this
 *     phase go to pendingActors[] instead of actors[], preventing iterator
 *     invalidation (adding to a list while iterating over it).
 *
 *   Phase 2 — Flush pending actors
 *     After all updates complete, pending actors get their world transform
 *     computed and are promoted to the active list. This is a common pattern
 *     in engines to avoid "one frame delay" bugs while keeping iteration safe.
 *
 *   Phase 3 — Destroy dead actors
 *     Iterates in reverse so swap-removal doesn't skip any actors.
 *     Dead actors are those with state == ACTOR_STATE_DEAD, typically set
 *     by gameplay logic during Phase 1.
 *------------------------------------------------------------------------------------*/
void GAME_FixedUpdate(Game* game, float deltaTime)
{
	assert(game != NULL);

    if (game->state != GAME_STATE_GAMEPLAY) return;

    if (LEVEL_MGR_IsTransitioning(&game->levelMgr)) return;

    /* ── Phase 1: Update active actors (guard pending additions) ──── */
    game->updatingActors = true;
    for (int i = 0; i < game->actorCount; i++) 
    {
        ACTOR_Update(game->actors[i], deltaTime);
    }
    game->updatingActors = false;

    /* ── Phase 2: Flush pending actors into the active list ───────── */
    for (int i = 0; i < game->pendingCount; i++) 
    {
        Actor *pending = game->pendingActors[i];
        ACTOR_ComputeWorldTransform(pending);
        if (game->actorCount < GAME_MAX_ACTORS) 
        {
            game->actors[game->actorCount++] = pending;
			game->pendingActors[i] = NULL;
        } 
        else 
        {
            TraceLog(LOG_WARNING, "GAME: Actor list full, destroying pending actor");
            ACTOR_Destroy(pending);
        }
    }
    game->pendingCount = 0;

    /* ── Phase 3: Destroy dead actors (reverse iteration for safe removal) */
    for (int i = game->actorCount - 1; i >= 0; i--) 
    {
        if (game->actors[i]->state == ACTOR_STATE_DEAD) 
        {
            ACTOR_Destroy(game->actors[i]);
        }
    }
}

/*------------------------------------------------------------------------------------
 * GAME_AddActor
 *
 *   Registers an actor with the game. Uses the double-buffer pattern:
 *   - If the game is currently iterating actors (updatingActors == true),
 *     the actor goes into the pending queue to avoid invalidating the iterator.
 *   - Otherwise, the actor is placed directly in the active list.
 *
 *   The actorsCreated counter increments unconditionally and serves as a
 *   lifetime diagnostic (total actors ever spawned, including destroyed ones).
 *------------------------------------------------------------------------------------*/

void GAME_AddActor(Game* game, Actor* actor) 
{
    if (!game || !actor) 
    {
        TraceLog(LOG_ERROR, "GAME_AddActor: game or actor pointer is NULL");
        return;
    }

    if (game->updatingActors) 
    {
        if (game->pendingCount < GAME_MAX_PENDING) 
        {
            game->pendingActors[game->pendingCount++] = actor;
        } 
        else 
        {
            TraceLog(LOG_WARNING, "GAME: Pending actor list full (%d)", GAME_MAX_PENDING);
        }
    }
    else
    {
        if (game->actorCount < GAME_MAX_ACTORS) 
        {
            game->actors[game->actorCount++] = actor;
        } 
        else 
        {
            TraceLog(LOG_WARNING, "GAME: Actor list full (%d)", GAME_MAX_ACTORS);
        }
    }
    game->actorsCreated++;
}

/*------------------------------------------------------------------------------------
 * GAME_RemoveActor
 *
 *   Unlinks an actor from whichever list contains it (active or pending).
 *   Does NOT destroy the actor — only removes the reference. Caller is
 *   responsible for calling ACTOR_Destroy if needed.
 *
 *   Uses linear search O(n) since actor lists are unordered.
 *------------------------------------------------------------------------------------*/
void GAME_RemoveActor(Game* game, Actor* actor)
{
    if (!game || !actor) 
    {
        TraceLog(LOG_ERROR, "GAME_RemoveActor: game or actor pointer is NULL");
        return;
    }

    for (int i = 0; i < game->actorCount; i++) 
    {
        if (game->actors[i] == actor) 
        {
            GAME_RemoveActiveActorByIndex(game, i);
            return;
        }
    }

    for(int i = 0; i < game->pendingCount; i++)
    {
        if (game->pendingActors[i] == actor)
        {
			GAME_RemovePendingActorByIndex(game, i);
            return;
        }
    }
}

/*------------------------------------------------------------------------------------
 * GAME_RemoveActiveActorByIndex (internal)
 *
 *   O(1) swap-remove: replaces the removed element with the last element,
 *   then decrements the count. This avoids shifting the entire array but
 *   does NOT preserve order — which is fine since the active actor list
 *   has no required ordering.
 *------------------------------------------------------------------------------------*/
void GAME_RemoveActiveActorByIndex(Game* game, int idx)
{
	assert(game != NULL);
    if (!game || idx < 0 || idx >= game->actorCount) return;

    game->actors[idx] = game->actors[game->actorCount - 1];
    game->actors[game->actorCount - 1] = NULL;
    game->actorCount--;
}

/*------------------------------------------------------------------------------------
 * GAME_RemovePendingActorByIndex (internal)
 *
 *   Same O(1) swap-remove as the active list variant.
 *------------------------------------------------------------------------------------*/
void GAME_RemovePendingActorByIndex(Game* game, int idx)
{
	assert(game != NULL);
    if (!game || idx < 0 || idx >= game->pendingCount) return;

    game->pendingActors[idx] = game->pendingActors[game->pendingCount - 1];
    game->pendingActors[game->pendingCount - 1] = NULL;
    game->pendingCount--;
}

/*------------------------------------------------------------------------------------
 * GAME_RemoveAllActors
 *
 *   Nuclear option: destroys every actor in both lists. Used during level
 *   teardown to ensure a clean slate. Iterates active list in reverse
 *   (ACTOR_Destroy may call GAME_RemoveActor internally via swap-remove,
 *   so reverse iteration keeps indices valid). Pending list iterates forward
 *   since those actors aren't in the active list.
 *------------------------------------------------------------------------------------*/
void GAME_RemoveAllActors(Game* game)
{
    for (int i = game->actorCount - 1; i >= 0; i--) 
    {
        ACTOR_Destroy(game->actors[i]);
    }

    for (int i = 0; i < game->pendingCount; i++) 
    {
        ACTOR_Destroy(game->pendingActors[i]);
    }
}

/*------------------------------------------------------------------------------------
 * GAME_ChangeLevel
 *
 *   Convenience wrapper that triggers a level transition using the default
 *   visual effects (fade out → load → fade in). The actual transition logic
 *   runs inside LEVEL_MGR_Update, which handles the asynchronous state machine
 *   for seamless scene changes without blocking the main loop.
 *------------------------------------------------------------------------------------*/
void GAME_ChangeLevel(Game* game, Level* level)
{
    if (!game || !level)
    {
        TraceLog(LOG_ERROR, "GAME_ChangeLevel: game or level pointer is NULL");
        return;
    }

    LEVEL_MGR_TransitionTo(&game->levelMgr, level, 
        TRANSITION_DEFAULT_EFFECT_OUT, 
        TRANSITION_DEFAULT_EFFECT_IN,
        TRANSITION_DEFAULT_DURATION);
}

/*------------------------------------------------------------------------------------
 * GAME_FindActorByTag
 *
 *   Searches both active and pending lists for the first actor whose tag
 *   bitmask matches the query. Tags are bitfields, so the match uses
 *   bitwise AND — an actor with tags=0x05 matches queries for 0x01 or 0x04.
 *
 *   Returns the first match found, or NULL if none match.
 *   For finding ALL matching actors, use GAME_FindActorsByTag instead.
 *------------------------------------------------------------------------------------*/
Actor* GAME_FindActorByTag(Game* game, unsigned int tag)
{
    if (!game)
    {
        TraceLog(LOG_ERROR, "GAME_FindActorByTag: game pointer is NULL");
        return NULL;
    }
    for (int i = 0; i < game->actorCount; i++)
    {
        if ((game->actors[i]->tags & tag) != 0)
        {
            return game->actors[i];
        }
    }
    for (int i = 0; i < game->pendingCount; i++)
    {
        if ((game->pendingActors[i]->tags & tag) != 0)
        {
            return game->pendingActors[i];
        }
    }
	return NULL;
}

/*------------------------------------------------------------------------------------
 * GAME_FindActorsByTag
 *
 *   Multi-result variant of tag search. Fills outArray with up to maxResults
 *   matching actor pointers and returns the count found. Searches both active
 *   and pending lists.
 *
 *   Usage:
 *       Actor* enemies[16];
 *       int count = GAME_FindActorsByTag(game, TAG_ENEMY, enemies, 16);
 *       for (int i = 0; i < count; i++) { ... }
 *------------------------------------------------------------------------------------*/
int GAME_FindActorsByTag(Game* game, unsigned int tag, Actor** outArray, int maxResults)
{
    if (!game || !outArray || maxResults <= 0)
    {
        TraceLog(LOG_ERROR, "GAME_FindActorsByTag: invalid arguments");
        return 0;
    }
    int count = 0;
    for (int i = 0; i < game->actorCount && count < maxResults; i++)
    {
        if ((game->actors[i]->tags & tag) != 0)
        {
            outArray[count++] = game->actors[i];
        }
    }
    for (int i = 0; i < game->pendingCount && count < maxResults; i++)
    {
        if ((game->pendingActors[i]->tags & tag) != 0)
        {
            outArray[count++] = game->pendingActors[i];
        }
    }
	return count;
}

/*------------------------------------------------------------------------------------
 * GAME_GetTime
 *
 *   Thin wrapper around Raylib's GetTime(), which returns seconds since
 *   InitWindow(). Exists so gameplay code doesn't need to include raylib.h
 *   directly and to provide a single point for future time manipulation
 *   (e.g. time scaling, pause-aware timers).
 *------------------------------------------------------------------------------------*/
float GAME_GetTime(Game* game)
{
    if (!game)
    {
        TraceLog(LOG_ERROR, "GAME_GetTime: game pointer is NULL");
        return 0.0f;
    }
	return GetTime();
}
