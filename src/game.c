#include "game.h"
#include <stdio.h>
#include <assert.h>

bool GAME_Init(Game* game) 
{
    if (!game)
    {
        TraceLog(LOG_ERROR, "GAME_Init: game pointer is NULL");
        return false;
    }

    game->state = GAME_STATE_INIT;
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

    TraceLog(LOG_INFO, "Game initialized - Updates: %dHz, Rendering: %dFPS",
        UPDATE_RATE, GetFPS());

	return true;
}

void GAME_ProcessInput(Game* game)
{
	assert(game != NULL);

    if (IsKeyPressed(KEY_RIGHT)) 
    {
        switch (game->state)
        {
        case GAME_STATE_INIT:
            game->state = GAME_STATE_MENU;
			break;
        case GAME_STATE_MENU:
            game->state = GAME_STATE_GAMEPLAY;
            break;
        case GAME_STATE_GAMEPLAY:
            game->state = GAME_STATE_GAME_OVER;
			break;
		case GAME_STATE_GAME_OVER:
			game->state = GAME_STATE_INIT;
			break;
        default:
            break;
        }
    }

    if (IsKeyPressed(KEY_LEFT)) 
    {
        switch (game->state)
        {
		case GAME_STATE_INIT:
			game->state = GAME_STATE_GAME_OVER;
			break;
        case GAME_STATE_MENU:
            game->state = GAME_STATE_INIT;
            break;
        case GAME_STATE_GAMEPLAY:
            game->state = GAME_STATE_MENU;
            break;
        case GAME_STATE_GAME_OVER:
            game->state = GAME_STATE_GAMEPLAY;
            break;
        default:
            break;
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) 
    {
		game->state = GAME_STATE_QUIT;
    }
}

void GAME_FixedUpdate(Game* game, float deltaTime)
{
	assert(game != NULL);

    if (game->state != GAME_STATE_GAMEPLAY)
    {
        return;
    }

	(void)deltaTime; /* Currently unused, but will be needed for actual game logic */
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
    switch (game->state) 
    {
	case GAME_STATE_INIT:    bg = (Color){ 20, 20, 20, 255 };  break;
	case GAME_STATE_MENU:    bg = (Color){ 20, 40, 20, 255 };  break;
    case GAME_STATE_GAMEPLAY: bg = (Color){ 20, 20, 40, 255 };  break;
    case GAME_STATE_PAUSED:  bg = (Color){ 40, 20, 20, 255 };  break;
	case GAME_STATE_GAME_OVER: bg = (Color){ 40, 40, 20, 255 };  break;
    default:                 bg = BLACK;                         break;
    }

    BeginDrawing();
        ClearBackground(bg);

        /* ── Debug HUD ── */
        int y = 20;
        const int step = 28;

        DrawText(TextFormat("Render FPS: %d  (target: %d)", GetFPS(), RENDER_FPS),
            20, y, 20, GREEN);
        y += step;

        DrawText(TextFormat("Frame dt: %.4f s", GetFrameTime()), 20, y, 20, GREEN);
        y += step;

        DrawText(TextFormat("Update Hz: %d  (fixed dt: %.4f s)", UPDATE_RATE, FIXED_TIMESTEP),
            20, y, 20, GREEN);
        y += step;

        DrawText(TextFormat("Fixed updates this frame: %d", game->updateCount), 20, y, 20,
            game->updateCount >= 2 ? ORANGE : GREEN);
        y += step;

        DrawText(TextFormat("Accumulator: %.4f s", game->accumulator), 20, y, 20, GRAY);
        y += step;

        DrawText(TextFormat("State: %s", StateName(game->state)), 20, y, 20, YELLOW);
        y += step * 2;

        /* ── Instructions ── */
        DrawText("Controls:", 20, y, 20, WHITE);
        y += step;
        DrawText("  ARROWS  - Toggle Gameplay / Paused", 20, y, 18, LIGHTGRAY);
        y += step;
        DrawText("  ESC    - Quit", 20, y, 18, LIGHTGRAY);
        y += step * 2;

        /* ── Timing explanation ── */
        if (RENDER_FPS < UPDATE_RATE) {
            DrawText(TextFormat(
                "Mode: %d render / %d update -> %d ticks per rendered frame",
                RENDER_FPS, UPDATE_RATE,
                UPDATE_RATE / RENDER_FPS),
                20, y, 18, SKYBLUE);
            y += step;
            DrawText("Gameplay at full speed, visuals at half framerate.",
                20, y, 18, SKYBLUE);
        }
        else {
            DrawText(TextFormat(
                "Mode: %d render / %d update -> 1 tick per rendered frame",
                RENDER_FPS, UPDATE_RATE),
                20, y, 18, SKYBLUE);
        }

        /* Paused overlay */
        if (game->state == GAME_STATE_PAUSED) {
            const char* msg = "PAUSED";
            int w = MeasureText(msg, 60);
            DrawText(msg,
                SCREEN_WIDTH / 2 - w / 2,
                SCREEN_HEIGHT / 2 - 30,
                60, RED);
        }

    EndDrawing();
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

    CloseWindow(),

    TraceLog(LOG_INFO, "Game shutdown - Time: %.2fs",
        GetTime());
}

//void GAME_AddActor(Game* game, Actor* actor) 
//{
//}

//void GAME_RemoveActor(Game* game, Actor* actor) 
//{
//}
