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

    MEMORY_Init(&game->memory);
    
	/* Initialize Raylib window and settings */
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, GAME_TITLE);
    if (!IsWindowReady())
    {
        TraceLog(LOG_ERROR, "Failed to initialize window");
        return false;
    }

    SetWindowState(FLAG_VSYNC_HINT);
	SetTargetFPS(RENDER_FPS);

    /* Subsystems */
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
    if (!game)
    {
        TraceLog(LOG_ERROR, "GAME_Shutdown: game pointer is NULL");
        return;
    }

    LEVEL_MGR_Shutdown(&game->levelMgr, game);
    RENDERER_Shutdown(&game->renderer);

    CloseWindow();

    MEMORY_Shutdown(&game->memory);
    
    TraceLog(LOG_INFO, "Game shutdown - Time: %.2fs",
        GetTime());
}

void GAME_Run(Game* game)
{
    if (!game)
    {
        TraceLog(LOG_ERROR, "GAME_Run: game pointer is NULL");
        return;
    }

    while (!WindowShouldClose() && game->state != GAME_STATE_QUIT)
    {
        float frameTime = GetFrameTime();
        if (frameTime > MAX_DELTA_TIME)
        {
            frameTime = MAX_DELTA_TIME;
        }

        /* Advance transition timer (even when paused) */
        LEVEL_MGR_Update(&game->levelMgr, game, frameTime);
		DEBUG_Update(game);
        GAME_ProcessInput(game);

        /* Update clear color based on game state */
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

        game->accumulator += frameTime;
		game->updateCount = 0;

        /* Save actor transform state before physics updates */
        for (int i = 0; i < game->actorCount; i++)
        {
            SCENE_COMPONENT_SavePrevState(&game->actors[i]->root);
        }

        while (game->accumulator >= FIXED_TIMESTEP)
        {
            GAME_FixedUpdate(game, FIXED_TIMESTEP);
            game->accumulator -= FIXED_TIMESTEP;
			game->updateCount++;
        }

        /* Interpolate actor transforms for smooth rendering between fixed updates.
         * alpha = 0.0 → show previous state, 1.0 → show current state. */
        float alpha = game->accumulator / FIXED_TIMESTEP;
        for (int i = 0; i < game->actorCount; i++)
        {
            SCENE_COMPONENT_InterpolateForRender(&game->actors[i]->root, alpha);
        }

        /* Render frame */
        RENDERER_DrawFrame(&game->renderer, game);

        /* Restore actual physics transforms after rendering */
        for (int i = 0; i < game->actorCount; i++)
        {
            SCENE_COMPONENT_RestoreFromInterpolation(&game->actors[i]->root);
        }
    }
}

void GAME_ProcessInput(Game* game)
{
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

    /* Block level and actor input during transitions */
    if (LEVEL_MGR_IsTransitioning(&game->levelMgr)) return;

    /* Level-specific input */
    Level* active = LEVEL_MGR_GetActiveLevel(&game->levelMgr);
    if (active && active->ProcessInput)
    {
        active->ProcessInput(game);
    }

    /* Dispatch to actors */
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
	assert(game != NULL);

    if (game->state != GAME_STATE_GAMEPLAY) return;

    /* Don't update gameplay during transitions */
    if (LEVEL_MGR_IsTransitioning(&game->levelMgr)) return;

	/* Phase 1: Update all active actors */
    game->updatingActors = true;
    for (int i = 0; i < game->actorCount; i++) 
    {
        ACTOR_Update(game->actors[i], deltaTime);
    }
    game->updatingActors = false;

    /* Phase 2: Move pending → active */
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

    /* Phase 3: Destroy dead actors (iterate backwards for safe swap-removal) */
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
	assert(game != NULL);
    if (!game || idx < 0 || idx >= game->actorCount) return;

    game->actors[idx] = game->actors[game->actorCount - 1];
    game->actors[game->actorCount - 1] = NULL;
    game->actorCount--;
}

void GAME_RemovePendingActorByIndex(Game* game, int idx)
{
	assert(game != NULL);
    if (!game || idx < 0 || idx >= game->pendingCount) return;

    game->pendingActors[idx] = game->pendingActors[game->pendingCount - 1];
    game->pendingActors[game->pendingCount - 1] = NULL;
    game->pendingCount--;
}

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