#include "actor.h"
#include "component.h"
#include "demo.h"
#include "game.h"
#include "rlgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void GAME_ProcessInput(Game* game);
static void GAME_FixedUpdate(Game* game, float deltaTime);
static void GAME_Render(Game* game);

static void GAME_RemoveActorByIndex(Game *game, int idx) 
{
    if(!game || idx < 0 || idx >= game->actorCount) return;

    game->actors[idx] = game->actors[game->actorCount - 1];
    game->actors[game->actorCount - 1] = NULL;
    game->actorCount--;
}

bool GAME_Init(Game* game) 
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

	/* Initialize Raylib window and settings */
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, GAME_TITLE);
    if (!IsWindowReady())
    {
        TraceLog(LOG_ERROR, "Failed to initialize window");
        return false;
    }

    SetWindowState(FLAG_VSYNC_HINT);
	SetTargetFPS(RENDER_FPS);

    DEMO_Init(game);
    TraceLog(LOG_INFO, "Game initialized - Updates: %dHz, Rendering: %dFPS",
        UPDATE_RATE, GetFPS());

	return true;
}

void GAME_Run(Game* game)
{
    if (!game)
    {
        TraceLog(LOG_ERROR, "GAME_RunLoop: game pointer is NULL");
        return;
    }

    while (!WindowShouldClose() && game->state != GAME_STATE_QUIT)
    {
        float frameTime = GetFrameTime();
        if (frameTime > MAX_DELTA_TIME)
        {
            frameTime = MAX_DELTA_TIME;
        }

        GAME_ProcessInput(game);

        game->accumulator += frameTime;
		game->updateCount = 0;

        while (game->accumulator >= FIXED_TIMESTEP)
        {
            GAME_FixedUpdate(game, FIXED_TIMESTEP);
            game->accumulator -= FIXED_TIMESTEP;
			game->updateCount++;
        }

        /* Render frame */
        GAME_Render(game);
    }
}

void GAME_Shutdown(Game* game) 
{
    if (!game)
    {
        TraceLog(LOG_ERROR, "GAME_Shutdown: game pointer is NULL");
        return;
    }

    DEMO_Shutdown(game);

    for (int i = game->actorCount - 1; i >= 0; i--) 
    {
        ACTOR_Destroy(game->actors[i]);
    }
    game->actorCount = 0;

    for( int i = 0; i < game->pendingCount; i++) 
    {
        ACTOR_Destroy(game->pendingActors[i]);
    }
    game->pendingCount = 0;

    CloseWindow();

    TraceLog(LOG_INFO, "Game shutdown - Time: %.2fs",
        GetTime());
}

void GAME_ProcessInput(Game* game)
{
	assert(game != NULL);

    if (IsKeyPressed(KEY_ESCAPE)) 
    {
        game->state = GAME_STATE_QUIT;
    }

    /* Demo-specific input */
    DEMO_ProcessInput(game);

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

    if (game->state != GAME_STATE_GAMEPLAY)
    {
        return;
    }

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
            Actor *dead = game->actors[i];
            GAME_RemoveActorByIndex(game, i);
            ACTOR_Destroy(dead);
        }
    }
}

static const char* StateName(GameState state) 
{
    switch (state) 
    {
	case GAME_STATE_INIT:    return "INIT";
	case GAME_STATE_MENU:    return "MENU";
    case GAME_STATE_GAMEPLAY: return "GAMEPLAY";
    case GAME_STATE_PAUSED:  return "PAUSED";
	case GAME_STATE_GAME_OVER: return "GAME_OVER";
    case GAME_STATE_QUIT:    return "QUIT";
    default:                 return "UNKNOWN";
    }
}

void GAME_Render(Game* game)
{
	assert(game != NULL);

    Color bg;
    switch (game->state) {
        case GAME_STATE_GAMEPLAY: bg = (Color){ 20, 20, 40, 255 };  break;
        case GAME_STATE_PAUSED:  bg = (Color){ 40, 20, 20, 255 };  break;
        default:                 bg = BLACK;                         break;
    }

    BeginDrawing();
    ClearBackground(bg);

    /* ── 3D scene ── */
    {
        Camera3D camera = {
            .position   = (Vector3){ 15.0f, 12.0f, 15.0f },
            .target     = (Vector3){ 0.0f, 0.0f, 0.0f },
            .up         = (Vector3){ 0.0f, 1.0f, 0.0f },
            .fovy       = 45.0f,
            .projection = CAMERA_PERSPECTIVE,
        };

        BeginMode3D(camera);
            DrawGrid(20, 1.0f);
            DEMO_Render3D(game);
        EndMode3D();
    }

    /* ── 2D HUD ── */
    {
        int y = 10;
        const int step = 22;

        /* Engine info */
        DrawText(TextFormat("FPS: %d (target: %d)", GetFPS(), RENDER_FPS),
                 10, y, 18, GREEN);
        y += step;

        DrawText(TextFormat("Update Hz: %d | Ticks this frame: %d",
                 UPDATE_RATE, game->updateCount),
                 10, y, 18, GREEN);
        y += step;

        DrawText(TextFormat("Actors: %d / %d  (total: %d)",
                 game->actorCount, GAME_MAX_ACTORS, game->actorsCreated),
                 10, y, 18, GREEN);
        y += step;

        DrawText(TextFormat("State: %s", StateName(game->state)),
                 10, y, 18, YELLOW);
        y += step * 2;

        /* Demo-specific HUD */
        y = DEMO_RenderHUD(game, y);

        /* Engine controls (always at the bottom of demo HUD) */
        DrawText("  ESC - Toggle pause   Q - Quit",
                 10, y, 16, LIGHTGRAY);
    }

    if (game->state == GAME_STATE_PAUSED) {
        const char *msg = "PAUSED";
        int w = MeasureText(msg, 60);
        DrawText(msg,
                 SCREEN_WIDTH / 2 - w / 2,
                 SCREEN_HEIGHT / 2 - 30,
                 60, RED);
    }

    EndDrawing();
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
        } else 
        {
            TraceLog(LOG_WARNING, "GAME: Pending actor list full (%d)", GAME_MAX_PENDING);
        }
    } else {
        if (game->actorCount < GAME_MAX_ACTORS) 
        {
            game->actors[game->actorCount++] = actor;
        } else 
        {
            TraceLog(LOG_WARNING, "GAME: Actor list full (%d)", GAME_MAX_ACTORS);
        }
    }
    game->actorsCreated++;
}

void GAME_RemoveActor(Game* game, Actor* actor) 
{
}
