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

static void GAME_ProcessInput(Game* game);
static void GAME_FixedUpdate(Game* game, float deltaTime);

static void GAME_RemoveActiveActorByIndex(Game* game, int idx);
static void GAME_RemovePendingActorByIndex(Game* game, int idx);

bool GAME_Init(Game* game, Level* initialLevel) 
{
    /*
     * Initialise the engine in this order (each step depends on the last):
     *   1. Zero the entire Game struct to avoid undefined fields.
     *   2. Initialise the memory system (pools must exist before any actor
     *      or component is created).
     *   3. Open the Raylib window (must exist before any rendering call).
     *   4. Initialise renderer, physics, debug, and level manager.
     *   5. Load the initial level.
     *
     * Returns false (without crashing) if a critical step fails, so that
     * main() can clean up gracefully.
     */
    if (!game)
    {
        TraceLog(LOG_ERROR, "GAME_Init: game pointer is NULL");
        return false;
    }

    memset(game, 0, sizeof(Game));
    game->state = GAME_STATE_GAMEPLAY;
	game->accumulator = 0.0f;
	game->updateCount = 0;

    MEMORY_Init(&game->memory);
    
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, GAME_TITLE);
    if (!IsWindowReady())
    {
        TraceLog(LOG_ERROR, "Failed to initialize window");
        return false;
    }

    SetWindowState(FLAG_VSYNC_HINT);
	SetTargetFPS(RENDER_FPS);

    RENDERER_Init(&game->renderer);
    PHYS_WORLD_Init(&game->physWorld);
    DEBUG_Init();
    LEVEL_MGR_Init(&game->levelMgr, game, initialLevel);

    TraceLog(LOG_INFO, "Game initialized - Updates: %dHz, Rendering: %dFPS",
        UPDATE_RATE, GetFPS());

	return true;
}

void GAME_Shutdown(Game* game)
{
    /*
     * Shutdown order is the reverse of init:
     *   1. Shut down the level manager (unloads the active level and
     *      destroys all its actors).
     *   2. Shut down physics and renderer.
     *   3. Close the Raylib window.
     *   4. Shut down the memory system last (all objects must be freed
     *      before the pools are destroyed).
     */
    if (!game)
    {
        TraceLog(LOG_ERROR, "GAME_Shutdown: game pointer is NULL");
        return;
    }

    LEVEL_MGR_Shutdown(&game->levelMgr, game);
    PHYS_WORLD_Shutdown(&game->physWorld);
    RENDERER_Shutdown(&game->renderer);

    CloseWindow();

    MEMORY_Shutdown(&game->memory);
    
    TraceLog(LOG_INFO, "Game shutdown - Time: %.2fs",
        GetTime());
}

void GAME_Run(Game* game)
{
    /*
     * Fixed-timestep main loop with visual interpolation.
     *
     * ── Step 1: Frame time & clamping ──────────────────────────────
     * Clamp frame time to MAX_DELTA_TIME to prevent the "spiral of
     * death" (a slow frame causing more updates, causing a slower next
     * frame, etc.).
     *
     * ── Step 2: Level & debug & input ──────────────────────────────
     * Level manager transition tick, debug system update, and global
     * input (pause, quit, level-specific input).
     *
     * ── Step 3: Save previous state ────────────────────────────────
     * Snapshot all actor root positions/rotations so that
     * InterpolateForRender can lerp between previous and current.
     *
     * ── Step 4: Fixed update loop ──────────────────────────────────
     * Drain the accumulator by running N fixed updates at FIXED_TIMESTEP.
     * Actor updates, physics, and pending-actor promotion happen here.
     *
     * ── Step 5: Visual interpolation ───────────────────────────────
     * Lerp all actor positions by alpha = accumulator / FIXED_TIMESTEP.
     * This gives sub-frame smooth rendering even when physics runs at 60Hz
     * and the display runs at 30Hz (or vice versa).
     *
     * ── Step 6: Render ─────────────────────────────────────────────
     * Draw the interpolated frame.
     *
     * ── Step 7: Restore physics state ──────────────────────────────
     * Put all actor positions back to the true physics state so the next
     * fixed update starts from the correct values.
     */
    if (!game)
    {
        TraceLog(LOG_ERROR, "GAME_Run: game pointer is NULL");
        return;
    }

    while (!WindowShouldClose() && game->state != GAME_STATE_QUIT)
    {
        /* ── Step 1: Frame time & clamping ───────────────────────── */
        float frameTime = GetFrameTime();
        if (frameTime > MAX_DELTA_TIME)
        {
            frameTime = MAX_DELTA_TIME;
        }

        /* ── Step 2: Level, debug, and input ─────────────────────── */
        LEVEL_MGR_Update(&game->levelMgr, game, frameTime);
		DEBUG_Update(game);
        GAME_ProcessInput(game);

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

        /* ── Step 3: Save previous state ─────────────────────────── */
        game->accumulator += frameTime;
		game->updateCount = 0;

        for (int i = 0; i < game->actorCount; i++)
        {
            SCENE_COMPONENT_SavePrevState(&game->actors[i]->root);
        }

        /* ── Step 4: Fixed update loop ───────────────────────────── */
        while (game->accumulator >= FIXED_TIMESTEP)
        {
            GAME_FixedUpdate(game, FIXED_TIMESTEP);
            game->accumulator -= FIXED_TIMESTEP;
			game->updateCount++;
        }

        /* ── Step 5: Visual interpolation ────────────────────────── */
        float alpha = game->accumulator / FIXED_TIMESTEP;
        for (int i = 0; i < game->actorCount; i++)
        {
            SCENE_COMPONENT_InterpolateForRender(&game->actors[i]->root, alpha);
        }

        /* ── Step 6: Render ──────────────────────────────────────── */
        RENDERER_DrawFrame(&game->renderer, game);

        /* ── Step 7: Restore physics state ───────────────────────── */
        for (int i = 0; i < game->actorCount; i++)
        {
            SCENE_COMPONENT_RestoreFromInterpolation(&game->actors[i]->root);
        }
    }
}

void GAME_ProcessInput(Game* game)
{
    /*
     * Handle global input (pause toggle, quit) first.  Then forward
     * to the active level's ProcessInput and to all active actors.
     * Input is skipped during level transitions to prevent control
     * bleed-through between levels.
     */
	assert(game != NULL);

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

void GAME_FixedUpdate(Game* game, float deltaTime)
{
    /*
     * One fixed-timestep simulation step.
     *
     * ── Step 1: Update all live actors ─────────────────────────────
     * Set updatingActors = true so any actor created during Update goes
     * to pendingActors[] instead of being inserted into the live array
     * mid-iteration.
     *
     * ── Step 2: Promote pending actors ─────────────────────────────
     * After the update pass, compute the world transform of each pending
     * actor and move it into the live array.  Destroy any that cannot fit.
     *
     * ── Step 3: Remove dead actors ─────────────────────────────────
     * Iterate backwards to destroy actors marked ACTOR_STATE_DEAD without
     * shifting live actors we have not yet visited.
     *
     * Skipped entirely if the game is paused or mid-transition.
     */
	assert(game != NULL);

    if (game->state != GAME_STATE_GAMEPLAY) return;

    if (LEVEL_MGR_IsTransitioning(&game->levelMgr)) return;

    /* ── Step 1: Update all live actors ─────────────────────────── */
    game->updatingActors = true;
    for (int i = 0; i < game->actorCount; i++) 
    {
        ACTOR_Update(game->actors[i], deltaTime);
    }
    game->updatingActors = false;

    /* ── Step 2: Promote pending actors ──────────────────────────── */
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

    /* ── Step 3: Remove dead actors ──────────────────────────────── */
    for (int i = game->actorCount - 1; i >= 0; i--) 
    {
        if (game->actors[i]->state == ACTOR_STATE_DEAD) 
        {
            ACTOR_Destroy(game->actors[i]);
        }
    }
}

void GAME_AddActor(Game* game, Actor* actor) 
{
    /*
     * Pending actor queue pattern: if an update pass is in progress
     * (updatingActors == true), defer the addition to pendingActors[] so
     * the current iteration over actors[] is not invalidated.
     * Otherwise add directly to the live list.
     */
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

void GAME_RemoveActor(Game* game, Actor* actor) 
{
    /*
     * Search both the live and pending arrays.  Uses
     * GAME_RemoveActiveActorByIndex / GAME_RemovePendingActorByIndex for
     * O(1) swap-remove once the index is found.
     */
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

void GAME_RemoveActiveActorByIndex(Game* game, int idx)
{
    /*
     * Swap-remove from the live array: replace slot idx with the last
     * entry and decrement actorCount.  Preserves array density without
     * shifting.
     */
	assert(game != NULL);
    if (!game || idx < 0 || idx >= game->actorCount) return;

    game->actors[idx] = game->actors[game->actorCount - 1];
    game->actors[game->actorCount - 1] = NULL;
    game->actorCount--;
}

void GAME_RemovePendingActorByIndex(Game* game, int idx)
{
    /*
     * Swap-remove from the pending array: replace slot idx with the last
     * pending entry and decrement pendingCount.
     */
	assert(game != NULL);
    if (!game || idx < 0 || idx >= game->pendingCount) return;

    game->pendingActors[idx] = game->pendingActors[game->pendingCount - 1];
    game->pendingActors[game->pendingCount - 1] = NULL;
    game->pendingCount--;
}

void GAME_RemoveAllActors(Game* game)
{
    /*
     * Iterate backwards through the live array (so swap-remove indices
     * stay valid) then destroy all pending actors.
     */
    for (int i = game->actorCount - 1; i >= 0; i--) 
    {
        ACTOR_Destroy(game->actors[i]);
    }

    for (int i = 0; i < game->pendingCount; i++) 
    {
        ACTOR_Destroy(game->pendingActors[i]);
    }
}

void GAME_ChangeLevel(Game* game, Level* level)
{
    /*
     * Delegate to the LevelManager which handles the fade-out, actor
     * cleanup, level swap, and fade-in sequence.
     */
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

Actor* GAME_FindActorByTag(Game* game, unsigned int tag)
{
    /*
     * Scan both live and pending arrays for the first actor whose tag
     * bitmask has any bit in common with tag.  O(n) worst case.
     */
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

int GAME_FindActorsByTag(Game* game, unsigned int tag, Actor** outArray, int maxResults)
{
    /*
     * Collect all matching actors (live then pending) into outArray.
     * Stops collecting once maxResults is reached.  Returns the count
     * of actors written.
     */
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

float GAME_GetTime(Game* game)
{
    if (!game)
    {
        TraceLog(LOG_ERROR, "GAME_GetTime: game pointer is NULL");
        return 0.0f;
    }
	return GetTime();
}
