#include "actor.h"
#include "component.h"
#include "debug.h"
#include "mesh_component.h"
#include "scene.h"
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void GAME_ProcessInput(Game* game);
static void GAME_FixedUpdate(Game* game, float deltaTime);
static void GAME_Render(Game* game);
static void GAME_DrawMeshComponents(Game* game); // TODO: Move to renderer subsystem if we add one
static void GAME_ApplyNextScene(Game* game); // TODO: Move to scene manager subsystem if we add one

bool GAME_Init(Game* game, Scene* initialScene) 
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

    DEBUG_Init();

    /* Initialize the first scene */
    game->activeScene = initialScene;
    if (initialScene && initialScene->Init) 
    {
        initialScene->Init(game);
    }

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

    GAME_RemoveAllActors(game);

    if (game->activeScene && game->activeScene->Shutdown) 
    {
        game->activeScene->Shutdown(game);
    }
    game->activeScene = NULL;

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
        if (game->nextScene) 
        {
            GAME_ApplyNextScene(game);
        }

        float frameTime = GetFrameTime();
        if (frameTime > MAX_DELTA_TIME)
        {
            frameTime = MAX_DELTA_TIME;
        }

		DEBUG_Update(game);
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

    /* Scene-specific input */
    if (game->activeScene && game->activeScene->ProcessInput) 
    {
        game->activeScene->ProcessInput(game);
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

static void GAME_DrawMeshComponents(Game* game) 
{
    for (int i = 0; i < game->actorCount; i++) 
    {
        Actor* actor = game->actors[i];
        if (actor->state != ACTOR_STATE_ACTIVE) continue;

        Component* comp = ACTOR_GetComponentOfType(actor, COMPONENT_MESH);
        if (comp) 
        {
            MESH_COMPONENT_Draw(comp);
        }
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
                .position = (Vector3){ 15.0f, 12.0f, 15.0f },
                .target = (Vector3){ 0.0f, 0.0f, 0.0f },
                .up = (Vector3){ 0.0f, 1.0f, 0.0f },
                .fovy = 45.0f,
                .projection = CAMERA_PERSPECTIVE,
            };

            BeginMode3D(camera);
                GAME_DrawMeshComponents(game);

                if (game->activeScene && game->activeScene->Render3D) 
                {
                    game->activeScene->Render3D(game);
                }
            EndMode3D();
        }

        /* ── 2D: Scene HUD ── */
        {
            int y = 10;

            if (game->activeScene && game->activeScene->RenderHUD) 
            {
                y = game->activeScene->RenderHUD(game, y);
            }

            DrawText("  ESC - Quit   P - Pause   F1 - Debug",
                10, y, 16, (Color) { 120, 120, 120, 200 });
        }

        /* ── 2D: Pause overlay ── */
        if (game->state == GAME_STATE_PAUSED) 
        {
            const char* msg = "PAUSED";
            int w = MeasureText(msg, 60);
            DrawText(msg,
                SCREEN_WIDTH / 2 - w / 2,
                SCREEN_HEIGHT / 2 - 30,
                60, RED);
        }

        /* ── 2D: Debug overlay (on top of everything) ── */
        DEBUG_Render(game);

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

void GAME_ChangeScene(Game* game, Scene* scene)
{
   if (!game || !scene) 
    {
        TraceLog(LOG_ERROR, "GAME_ChangeScene: game or scene pointer is NULL");
        return;
    }

   game->nextScene = scene;
}

static void GAME_ApplyNextScene(Game* game) 
{
	assert(game != NULL);

    Scene* newScene = game->nextScene;
    game->nextScene = NULL;

    /* Shutdown current */
    GAME_RemoveAllActors(game);
    if (game->activeScene && game->activeScene->Shutdown) 
    {
        game->activeScene->Shutdown(game);
    }

    /* Reset state for the new scene */
    game->state = GAME_STATE_GAMEPLAY;
    game->accumulator = 0.0f;
    game->actorsCreated = 0;

    /* Init new */
    game->activeScene = newScene;
    if (newScene && newScene->Init) 
    {
        newScene->Init(game);
    }
}