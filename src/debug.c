#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "debug.h"
#include "game.h"
#include "actor.h"
#include "scene.h"
#include "raylib.h"

#define FPS_HISTORY_SIZE 120  /* 2 seconds at 60fps */

typedef struct
{
    bool visible;
    float fpsHistory[FPS_HISTORY_SIZE];
    int fpsIndex;
    float frameTimeMs;
} DebugData;

static DebugData Debug;

void DEBUG_Init(void) 
{
    Debug.visible = false;
    Debug.fpsIndex = 0;
    Debug.frameTimeMs = 0.0f;

    for (int i = 0; i < FPS_HISTORY_SIZE; i++) 
    {
        Debug.fpsHistory[i] = 0.0f;
    }

    /* raygui styling — dark theme to not clash with game visuals */
    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);
}

void DEBUG_Update(Game* game) 
{
    (void)game;

    if (IsKeyPressed(KEY_F1)) 
    {
        Debug.visible = !Debug.visible;
    }

    Debug.frameTimeMs = GetFrameTime() * 1000.0f;
    Debug.fpsHistory[Debug.fpsIndex] = (float)GetFPS();
    Debug.fpsIndex = (Debug.fpsIndex + 1) % FPS_HISTORY_SIZE;
}

static void DEBUG_DrawFPSGraph(int x, int y, int width, int height, int targetFps) 
{
    /* Background */
    DrawRectangle(x, y, width, height, (Color) { 0, 0, 0, 160 });
    DrawRectangleLines(x, y, width, height, (Color) { 80, 80, 80, 200 });

    /* Target line */
    float target_ratio = (float)targetFps / (float)(targetFps + 20);
    int target_y = y + height - (int)(target_ratio * height);
    DrawLine(x, target_y, x + width, target_y, (Color) { 100, 100, 100, 150 });

    /* Bars */
    float bar_width = (float)width / FPS_HISTORY_SIZE;
    float max_fps = (float)(targetFps + 20);

    for (int i = 0; i < FPS_HISTORY_SIZE; i++) 
    {
        int idx = (Debug.fpsIndex + i) % FPS_HISTORY_SIZE;
        float fps = Debug.fpsHistory[idx];
        if (fps <= 0.0f) continue;

        float ratio = fps / max_fps;
        if (ratio > 1.0f) ratio = 1.0f;

        int bar_h = (int)(ratio * height);
        int bar_x = x + (int)(i * bar_width);
        int bar_y = y + height - bar_h;

        Color color;
        if (fps >= targetFps * 0.95f)      color = GREEN;
        else if (fps >= targetFps * 0.75f)  color = YELLOW;
        else                                  color = RED;

        DrawRectangle(bar_x, bar_y, (int)bar_width + 1, bar_h, color);
    }
}

static const char* StateName(GameState state) 
{
    switch (state) 
    {
    case GAME_STATE_GAMEPLAY: return "GAMEPLAY";
    case GAME_STATE_PAUSED:  return "PAUSED";
    case GAME_STATE_QUIT:    return "QUIT";
    default:                 return "UNKNOWN";
    }
}

void DEBUG_Render(Game* game) 
{
    if (!Debug.visible) 
    {
        DrawText("F1: Debug", GetScreenWidth() - 90, 10, 14, (Color) { 100, 100, 100, 150 });
        return;
    }

    const int panel_x = GetScreenWidth() - 280;
    const int panel_y = 10;
    const int panel_w = 270;
    const int row = 20;
    int y = panel_y;

    /* Panel background */
    DrawRectangle(panel_x - 5, panel_y - 5, panel_w + 10, 340,
        (Color) {
        10, 10, 10, 200
    });
    DrawRectangleLines(panel_x - 5, panel_y - 5, panel_w + 10, 340,
        (Color) {
        80, 80, 80, 200
    });

    DrawText("DEBUG", panel_x, y, 16, WHITE);
    DrawText("[F1 toggle]", panel_x + 60, y + 2, 12, GRAY);
    y += row + 4;

    DrawText(TextFormat("FPS: %d  (%.1f ms)", GetFPS(), Debug.frameTimeMs),
        panel_x, y, 14, GREEN);
    y += row;

    DrawText(TextFormat("Target: %d fps", RENDER_FPS),
        panel_x, y, 14, GRAY);
    y += row + 4;

    DEBUG_DrawFPSGraph(panel_x, y, panel_w, 60, RENDER_FPS);
    y += 60 + 8;

    /* ── Timing section ── */
    GuiLine((Rectangle) { panel_x, y, panel_w, 1 }, "Timing");
    y += row;

    DrawText(TextFormat("Update Hz: %d  (%0.4f s)",
        UPDATE_RATE, FIXED_TIMESTEP),
        panel_x, y, 14, LIGHTGRAY);
    y += row;

    DrawText(TextFormat("Ticks this frame: %d", game->updateCount),
        panel_x, y, 14, LIGHTGRAY);
    y += row;

    DrawText(TextFormat("Accumulator: %.4f s", game->accumulator),
        panel_x, y, 14, LIGHTGRAY);
    y += row + 4;

    /* ── Scene & Actors ── */
    GuiLine((Rectangle) { panel_x, y, panel_w, 1 }, "World");
    y += row;

    const char* scene_name = game->activeScene ? game->activeScene->name : "(none)";
    DrawText(TextFormat("Scene: %s", scene_name),
        panel_x, y, 14, YELLOW);
    y += row;

    DrawText(TextFormat("State: %s", StateName(game->state)),
        panel_x, y, 14, YELLOW);
    y += row;

    DrawText(TextFormat("Actors: %d / %d",
        game->actorCount, GAME_MAX_ACTORS),
        panel_x, y, 14, LIGHTGRAY);
    y += row;

    DrawText(TextFormat("Pending: %d / %d",
        game->pendingCount, GAME_MAX_PENDING),
        panel_x, y, 14, LIGHTGRAY);
    y += row;

    DrawText(TextFormat("Total created: %d", game->actorsCreated),
        panel_x, y, 14, LIGHTGRAY);
}

bool DEBUG_IsVisible(void)
{
    return Debug.visible;
}

void DEBUG_SetVisible(bool visible)
{
    Debug.visible = visible;
}