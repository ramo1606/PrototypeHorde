#include "actor.h"
#include "game.h"
#include "rlgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct 
{
    Actor base;     /* MUST be first */
} SpinnerActor;

static void spinner_update(Actor *self, float dt) 
{
    Quaternion rot = self->rotation;
    /* Rotate around Y axis (up) */
    Quaternion inc = QuaternionFromAxisAngle(
        (Vector3){ 0.0f, 1.0f, 0.0f }, 1.5f * dt);
    ACTOR_SetRotation(self, QuaternionMultiply(rot, inc));
}

static SpinnerActor *SpinnerActor_Create(Game *game) 
{
    SpinnerActor *self = malloc(sizeof(SpinnerActor));
    if (!self) return NULL;
    ACTOR_Init(&self->base, game);
    self->base.onUpdate = spinner_update;
    return self;
}

typedef struct 
{
    Actor base;     /* MUST be first */
    float bob_time;
    float base_y;   /* rest height */
} BobberActor;

static void bobber_update(Actor *self, float dt) 
{
    BobberActor *bobber = (BobberActor *)self;  /* safe: Actor is first field */
    bobber->bob_time += dt;
    Vector3 pos = self->position;
    pos.y = bobber->base_y + sinf(bobber->bob_time * 2.0f) * 0.5f;
    ACTOR_SetPosition(self, pos);
}

static BobberActor *BobberActor_Create(Game *game, float base_y) 
{
    BobberActor *self = malloc(sizeof(BobberActor));
    if (!self) return NULL;
    ACTOR_Init(&self->base, game);
    self->bob_time = 0.0f;
    self->base_y   = base_y;
    self->base.onUpdate = bobber_update;
    return self;
}

static void GAME_LoadData(Game *game);
static void GAME_UnloadData(Game *game);

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

    GAME_LoadData(game);

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

    GAME_UnloadData(game);
    CloseWindow(),

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

    if (IsKeyPressed(KEY_P)) 
    {
        if (game->state == GAME_STATE_GAMEPLAY)
            game->state = GAME_STATE_PAUSED;
        else if (game->state == GAME_STATE_PAUSED)
            game->state = GAME_STATE_GAMEPLAY;
    }

    /* Demo: spawn a spinner at random position on the XZ ground plane */
    if (IsKeyPressed(KEY_SPACE) && game->state == GAME_STATE_GAMEPLAY) 
    {
        SpinnerActor *s = SpinnerActor_Create(game);
        if (s) 
        {
            float x = (float)(GetRandomValue(-8, 8));
            float z = (float)(GetRandomValue(-8, 8));
            ACTOR_SetPosition(&s->base, (Vector3){ x, 0.5f, z });
            ACTOR_SetScale(&s->base, 0.6f);
        }
    }

    /* Demo: kill last actor */
    if (IsKeyPressed(KEY_BACKSPACE) && game->state == GAME_STATE_GAMEPLAY) 
    {
        if (game->actorCount > 0) 
        {
            game->actors[game->actorCount - 1]->state = ACTOR_STATE_DEAD;
        }
    }

    /* Dispatch input to active actors */
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

static void Game_DebugDrawActors(Game *game) 
{
    for (int i = 0; i < game->actorCount; i++) 
    {
        Actor *actor = game->actors[i];
        if (actor->state != ACTOR_STATE_ACTIVE) continue;

        /* Color by behavior so we can tell them apart */
        Color color = GRAY;
        if (actor->onUpdate == spinner_update) color = RED;
        else if (actor->onUpdate == bobber_update) color = SKYBLUE;

        /* Apply full world transform and draw a unit cube */
        rlPushMatrix();
            rlMultMatrixf(MatrixToFloatV(actor->worldTransform).v);
            DrawCube((Vector3){ 0 }, 1.0f, 1.0f, 1.0f, color);
            DrawCubeWires((Vector3){ 0 }, 1.0f, 1.0f, 1.0f, BLACK);
        rlPopMatrix();

        /* Forward direction indicator */
        Vector3 fwd = ACTOR_GetForward(actor);
        Vector3 start = actor->position;
        Vector3 end = Vector3Add(start, Vector3Scale(fwd, 1.5f));
        DrawLine3D(start, end, YELLOW);
    }
}

void GAME_Render(Game* game)
{
	assert(game != NULL);

    Color bg;
    switch (game->state) 
    {
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
                DrawGrid(20, 1.0f);     /* XZ plane grid — native Raylib, Y-up */
                Game_DebugDrawActors(game);
            EndMode3D();
        }

        /* ── 2D debug HUD ── */
        {
            int y = 10;
            const int step = 22;

            DrawText(TextFormat("FPS: %d (target: %d)", GetFPS(), RENDER_FPS),
                    10, y, 18, GREEN);
            y += step;

            DrawText(TextFormat("Update Hz: %d | Ticks this frame: %d",
                    UPDATE_RATE, game->updateCount),
                    10, y, 18, GREEN);
            y += step;

            DrawText(TextFormat("Actors: %d / %d  (pending: %d, total: %d)",
                    game->actorCount, GAME_MAX_ACTORS,
                    game->pendingCount, game->actorsCreated),
                    10, y, 18, GREEN);
            y += step;

            DrawText(TextFormat("State: %s", StateName(game->state)),
                    10, y, 18, YELLOW);
            y += step * 2;

            DrawText("Controls:", 10, y, 18, WHITE);
            y += step;
            DrawText("  SPACE     - Spawn spinning actor", 10, y, 16, LIGHTGRAY);
            y += step;
            DrawText("  BACKSPACE - Kill last actor", 10, y, 16, LIGHTGRAY);
            y += step;
            DrawText("  ESC       - Toggle pause", 10, y, 16, LIGHTGRAY);
            y += step;
            DrawText("  Q         - Quit", 10, y, 16, LIGHTGRAY);
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

static void GAME_LoadData(Game *game) 
{
    SetRandomSeed(GetTime());

    /*
     * Demo scene on the XZ ground plane (Y-up):
     *   - 5 static cubes along X axis (gray)
     *   - 1 spinning actor (red)
     *   - 1 bobbing actor (blue, shows embedding with derived fields)
     */

    /* Static cubes in a row along X */
    for (int i = 0; i < 5; i++) 
    {
        Actor *a = malloc(sizeof(Actor));
        ACTOR_Init(a, game);
        ACTOR_SetPosition(a, (Vector3){ (float)(i * 3 - 6), 0.5f, 0.0f });
    }

    /* Spinning actor — uses SpinnerActor embedding */
    SpinnerActor *spinner = SpinnerActor_Create(game);
    ACTOR_SetPosition(&spinner->base, (Vector3){ 0.0f, 0.5f, 4.0f });
    ACTOR_SetScale(&spinner->base, 1.5f);

    /* Bobbing actor — uses BobberActor embedding with derived field */
    BobberActor *bobber = BobberActor_Create(game, 1.0f);
    ACTOR_SetPosition(&bobber->base, (Vector3){ 0.0f, 1.0f, -4.0f });
}

static void GAME_UnloadData(Game *game) 
{
    for (int i = game->actorCount - 1; i >= 0; i--) 
    {
        ACTOR_Destroy(game->actors[i]);
    }
    game->actorCount = 0;

    for (int i = 0; i < game->pendingCount; i++) 
    {
        ACTOR_Destroy(game->pendingActors[i]);
    }
    game->pendingCount = 0;
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
