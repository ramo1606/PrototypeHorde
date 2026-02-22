#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "debug.h"
#include "game.h"
#include "actor.h"
#include "level.h"
#include "level_manager.h"
#include "renderer.h"
#include "memory.h"
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

    const int panelX = GetScreenWidth() - 280;
    const int panelY = 10;
    const int panelW = 270;
    const int row = 20;
    int y = panelY;

    /* Panel background */
    DrawRectangle(panelX - 5, panelY - 5, panelW + 10, 500,
        (Color) {
        10, 10, 10, 200
    });
    DrawRectangleLines(panelX - 5, panelY - 5, panelW + 10, 500,
        (Color) {
        80, 80, 80, 200
    });

    DrawText("DEBUG", panelX, y, 16, WHITE);
    DrawText("[F1 toggle]", panelX + 60, y + 2, 12, GRAY);
    y += row + 4;

    DrawText(TextFormat("FPS: %d  (%.1f ms)", GetFPS(), Debug.frameTimeMs),
        panelX, y, 14, GREEN);
    y += row;

    DrawText(TextFormat("Target: %d fps", RENDER_FPS),
        panelX, y, 14, GRAY);
    y += row + 4;

    DEBUG_DrawFPSGraph(panelX, y, panelW, 60, RENDER_FPS);
    y += 60 + 8;

    /* ── Timing section ── */
    GuiLine((Rectangle) { panelX, y, panelW, 1 }, "Timing");
    y += row;

    DrawText(TextFormat("Update Hz: %d  (%0.4f s)",
        UPDATE_RATE, FIXED_TIMESTEP),
        panelX, y, 14, LIGHTGRAY);
    y += row;

    DrawText(TextFormat("Ticks this frame: %d", game->updateCount),
        panelX, y, 14, LIGHTGRAY);
    y += row;

    DrawText(TextFormat("Accumulator: %.4f s", game->accumulator),
        panelX, y, 14, LIGHTGRAY);
    y += row + 4;

    /* ── Level & Actors ── */
    GuiLine((Rectangle) { panelX, y, panelW, 1 }, "World");
    y += row;

    Level* activeLevel = LEVEL_MGR_GetActiveLevel(&game->levelMgr);
    const char* levelName = activeLevel ? activeLevel->name : "(none)";
    DrawText(TextFormat("Level: %s", levelName),
        panelX, y, 14, YELLOW);
    y += row;

    DrawText(TextFormat("State: %s", StateName(game->state)),
        panelX, y, 14, YELLOW);
    y += row;

    DrawText(TextFormat("Actors: %d / %d",
        game->actorCount, GAME_MAX_ACTORS),
        panelX, y, 14, LIGHTGRAY);
    y += row;

    DrawText(TextFormat("Pending: %d / %d",
        game->pendingCount, GAME_MAX_PENDING),
        panelX, y, 14, LIGHTGRAY);
    y += row;

    DrawText(TextFormat("Total created: %d", game->actorsCreated),
        panelX, y, 14, LIGHTGRAY);
    y += row + 4;

    /* ── Rendering ── */
    GuiLine((Rectangle) { panelX, y, panelW, 1 }, "Rendering");
    y += row;

    DrawText(TextFormat("Meshes: %d registered", game->renderer.meshCount),
        panelX, y, 14, LIGHTGRAY);
    y += row;

    DrawText(TextFormat("Drawn: %d  Culled: %d",
        game->renderer.statsDrawn, game->renderer.statsCulled),
        panelX, y, 14, LIGHTGRAY);
    y += row;

    DrawText(TextFormat("Colliders: %d box  %d sphere",
        game->physWorld.boxCount, game->physWorld.sphereCount),
        panelX, y, 14, LIGHTGRAY);
    y += row + 4;

    /* ── Transition ── */
    if (LEVEL_MGR_IsTransitioning(&game->levelMgr))
    {
        GuiLine((Rectangle) { panelX, y, panelW, 1 }, "Transition");
        y += row;

        DrawText(TextFormat("State: %s", LEVEL_MGR_GetStateName(&game->levelMgr)),
            panelX, y, 14, ORANGE);
        y += row;

        DrawText(TextFormat("Progress: %.0f%%", game->levelMgr.progress * 100.0f),
            panelX, y, 14, ORANGE);
        y += row;

        const char* pendingName = game->levelMgr.pendingLevel
            ? game->levelMgr.pendingLevel->name : "(swapped)";
        DrawText(TextFormat("Target: %s", pendingName),
            panelX, y, 14, ORANGE);
        y += row + 4;
    }

    /* ── Memory ── */
    GuiLine((Rectangle){ panelX, y, panelW, 1 }, "Memory");
    y += row;

    /* Actor pool: total - free = used */
    int actorPoolTotal = (int)game->memory.actorPool.memSize;
    int actorPoolFree  = (int)game->memory.actorPool.freeBlocks;
    int actorPoolUsed  = actorPoolTotal - actorPoolFree;

    Color actorColor = LIGHTGRAY;
    float actorUsage = (actorPoolTotal > 0) ? (float)actorPoolUsed / actorPoolTotal : 0;
    if (actorUsage > 0.9f) actorColor = RED;
    else if (actorUsage > 0.7f) actorColor = YELLOW;

    DrawText(TextFormat("Actor pool: %d / %d", actorPoolUsed, actorPoolTotal),
        panelX, y, 14, actorColor);
    y += row;

    /* Component pool: free memory */
    size_t compFree = GetMemPoolFreeMemory(game->memory.componentPool);
    size_t compTotal = COMPONENT_POOL_BYTES;
    size_t compUsed = compTotal - compFree;

    Color compColor = LIGHTGRAY;
    float compUsage = (compTotal > 0) ? (float)compUsed / compTotal : 0;
    if (compUsage > 0.9f) compColor = RED;
    else if (compUsage > 0.7f) compColor = YELLOW;

    DrawText(TextFormat("Comp pool: %zu / %zu bytes", compUsed, compTotal),
        panelX, y, 14, compColor);
}

bool DEBUG_IsVisible(void)
{
    return Debug.visible;
}

void DEBUG_SetVisible(bool visible)
{
    Debug.visible = visible;
}