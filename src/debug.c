#include "debug.h"

#ifdef DEBUG_ENABLED

#include "game.h"
#include "level_manager.h"
#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <string.h>

#define FPS_HISTORY_SIZE 120    /* 2 seconds at 60fps */

static bool visible = false;

static struct
{
    const char*      name;
    DebugPanelDrawFn drawFn;
    DebugRender3DFn  render3DFn;
    bool             active;
} panels[DEBUG_MAX_PANELS];

static DebugPerfStats perf;

static float fpsHistory[FPS_HISTORY_SIZE];
static int   fpsIndex = 0;

/* ── FPS Graph ───────────────────────────────────────────────────────────── */

static void drawFPSGraph(int x, int y, int w, int h)
{
    DrawRectangle(x, y, w, h, (Color) { 0, 0, 0, 160 });
    DrawRectangleLines(x, y, w, h, (Color) { 80, 80, 80, 200 });

    float targetRatio = (float)RENDER_FPS / (float)(RENDER_FPS + 20);
    int targetY = y + h - (int)(targetRatio * h);
    DrawLine(x, targetY, x + w, targetY, (Color) { 100, 100, 100, 150 });

    float barW = (float)w / FPS_HISTORY_SIZE;
    float maxFps = (float)(RENDER_FPS + 20);

    for (int i = 0; i < FPS_HISTORY_SIZE; i++)
    {
        int idx = (fpsIndex + i) % FPS_HISTORY_SIZE;
        float fps = fpsHistory[idx];
        if (fps <= 0.0f) continue;

        float ratio = fps / maxFps;
        if (ratio > 1.0f) ratio = 1.0f;

        int barH = (int)(ratio * h);
        int barX = x + (int)(i * barW);
        int barY = y + h - barH;

        Color color;
        if (fps >= RENDER_FPS * 0.95f)      color = GREEN;
        else if (fps >= RENDER_FPS * 0.75f) color = YELLOW;
        else                                color = RED;

        DrawRectangle(barX, barY, (int)barW + 1, barH, color);
    }
}

/* ── Built-in: Performance Panel (F2) ────────────────────────────────────── */

static void drawPerfPanel(float x, float y, float w, float h)
{
    GuiGroupBox((Rectangle) { x, y, w, h }, "Performance (F2)");

    float row = y + 20;
    float pad = 10;

    GuiLabel((Rectangle) { x + pad, row, w - 2 * pad, 18 },
        TextFormat("FPS: %d  (%.1f ms)", perf.fps, perf.frametimeMs));
    row += 18;
    GuiLabel((Rectangle) { x + pad, row, w - 2 * pad, 18 },
        TextFormat("Avg: %.1f  Min: %.1f  Max: %.1f",
            perf.frametimeAvg, perf.frametimeMin, perf.frametimeMax));
    row += 18;
    GuiLabel((Rectangle) { x + pad, row, w - 2 * pad, 18 },
        TextFormat("Ticks/frame: %d  Alpha: %.2f",
            perf.ticksThisFrame, perf.alpha));
    row += 24;

    drawFPSGraph((int)(x + pad), (int)row, (int)(w - 2 * pad), 50);
    row += 56;

    size_t permUsed  = perf.arenaPermanentTotal - perf.arenaPermanentFree;
    size_t levelUsed = perf.arenaLevelTotal     - perf.arenaLevelFree;
    size_t scrUsed   = perf.arenaScratchTotal   - perf.arenaScratchFree;
    float bw = w - 2 * pad - 80;

    GuiLabel((Rectangle) { x + pad, row, w - 2 * pad, 14 }, "Permanent:");
    row += 14;
    float pv = (perf.arenaPermanentTotal > 0) ? (float)permUsed / (float)perf.arenaPermanentTotal : 0;
    GuiProgressBar((Rectangle) { x + pad, row, bw, 14 }, NULL,
        TextFormat("%zuK/%zuK", permUsed / 1024, perf.arenaPermanentTotal / 1024),
        &pv, 0, 1);
    row += 20;

    GuiLabel((Rectangle) { x + pad, row, w - 2 * pad, 14 }, "Level:");
    row += 14;
    float lv = (perf.arenaLevelTotal > 0) ? (float)levelUsed / (float)perf.arenaLevelTotal : 0;
    GuiProgressBar((Rectangle) { x + pad, row, bw, 14 }, NULL,
        TextFormat("%zuK/%zuK", levelUsed / 1024, perf.arenaLevelTotal / 1024),
        &lv, 0, 1);
    row += 20;

    GuiLabel((Rectangle) { x + pad, row, w - 2 * pad, 14 }, "Scratch:");
    row += 14;
    float sv = (perf.arenaScratchTotal > 0) ? (float)scrUsed / (float)perf.arenaScratchTotal : 0;
    GuiProgressBar((Rectangle) { x + pad, row, bw, 14 }, NULL,
        TextFormat("%zuK/%zuK", scrUsed / 1024, perf.arenaScratchTotal / 1024),
        &sv, 0, 1);
    row += 20;
}

/* ── Built-in: Renderer Panel (F3) ───────────────────────────────────────── */

static void drawRendererPanel(float x, float y, float w, float h)
{
    GuiGroupBox((Rectangle) { x, y, w, h }, "Renderer (F3)");

    float row = y + 20;
    float pad = 10;

    GuiLabel((Rectangle) { x + pad, row, w - 2 * pad, 18 },
        TextFormat("Renderables: %d / %d", perf.renderableCount, RENDERER_MAX_RENDERABLES));
    row += 22;

    GuiLabel((Rectangle) { x + pad, row, w - 2 * pad, 18 },
        TextFormat("Draw list: %d", perf.drawCount));
    row += 22;

    GuiLabel((Rectangle) { x + pad, row, w - 2 * pad, 18 },
        TextFormat("Drawn: %d  Culled: %d", perf.statsDrawn, perf.statsCulled));
    row += 22;

    int total = perf.statsDrawn + perf.statsCulled;
    float cullRatio = (total > 0) ? (float)perf.statsCulled / (float)total : 0.0f;

    GuiLabel((Rectangle) { x + pad, row, w - 2 * pad, 14 }, "Cull ratio:");
    row += 14;
    float bw = w - 2 * pad - 80;
    GuiProgressBar((Rectangle) { x + pad, row, bw, 14 }, NULL,
        TextFormat("%.0f%%", cullRatio * 100.0f),
        &cullRatio, 0, 1);
    row += 22;
}

/* ── Built-in: Physics Panel (F4) ────────────────────────────────────────── */

static void drawPhysicsPanel(float x, float y, float w, float h)
{
    GuiGroupBox((Rectangle) { x, y, w, h }, "Physics (F4)");

    float row = y + 20;
    float pad = 10;

    GuiLabel((Rectangle) { x + pad, row, w - 2 * pad, 18 },
        TextFormat("Colliders: %d / %d", perf.colliderCount, MAX_COLLIDERS));
    row += 22;

    GuiLabel((Rectangle) { x + pad, row, w - 2 * pad, 18 },
        TextFormat("Pairs checked: %d", perf.pairsChecked));
    row += 22;

    GuiLabel((Rectangle) { x + pad, row, w - 2 * pad, 18 },
        TextFormat("Contacts: %d", perf.contactsFound));
    row += 22;

    GuiLabel((Rectangle) { x + pad, row, w - 2 * pad, 18 },
        TextFormat("Triggers: %d", perf.triggersFound));
    row += 22;
}

/* ── Public API ──────────────────────────────────────────────────────────── */

void DebugInit(void)
{
    memset(panels, 0, sizeof(panels));
    memset(fpsHistory, 0, sizeof(fpsHistory));
    fpsIndex = 0;
    visible = false;

    DebugRegisterPanel(0, "Performance", drawPerfPanel);
    DebugRegisterPanel(1, "Renderer",    drawRendererPanel);
    DebugRegisterPanel(2, "Physics",     drawPhysicsPanel);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 13);
}

void DebugShutdown(void)
{
    /* Nothing to clean up */
}

void DebugRegisterPanel(int slot, const char* name, DebugPanelDrawFn drawFn)
{
    if (slot < 0 || slot >= DEBUG_MAX_PANELS) return;
    panels[slot].name   = name;
    panels[slot].drawFn = drawFn;
    panels[slot].active = false;
}

void DebugRegister3D(int slot, DebugRender3DFn renderFn)
{
    if (slot < 0 || slot >= DEBUG_MAX_PANELS) return;
    panels[slot].render3DFn = renderFn;
}

void DebugUpdate(Game* game)
{
    (void)game;

    if (IsKeyPressed(KEY_F1)) visible = !visible;

    fpsHistory[fpsIndex] = (float)GetFPS();
    fpsIndex = (fpsIndex + 1) % FPS_HISTORY_SIZE;

    if (!visible) return;

    if (IsKeyPressed(KEY_F2)) panels[0].active = !panels[0].active;
    if (IsKeyPressed(KEY_F3)) panels[1].active = !panels[1].active;
    if (IsKeyPressed(KEY_F4)) panels[2].active = !panels[2].active;
    if (IsKeyPressed(KEY_F5)) panels[3].active = !panels[3].active;
    if (IsKeyPressed(KEY_F6)) panels[4].active = !panels[4].active;
    if (IsKeyPressed(KEY_F7)) panels[5].active = !panels[5].active;
}

void DebugRender3D(Game* game)
{
    if (!visible) return;

    for (int i = 0; i < DEBUG_MAX_PANELS; i++)
    {
        if (panels[i].active && panels[i].render3DFn)
            panels[i].render3DFn(game);
    }
}

void DebugRender(Game* game)
{
    (void)game;

    if (!visible)
    {
        DrawText("F1: Debug", GetScreenWidth() - 90, 10, 14,
            (Color) { 100, 100, 100, 150 });
        return;
    }

    DrawRectangle(0, 0, GetScreenWidth(), 18, (Color) { 0, 0, 0, 160 });
    DrawText("DEBUG | F1 hide | F2-F7 panels", 10, 2, 14, GREEN);

    LevelManager* mgr = &((Game*)game)->levelMgr;
    Level* active = LevelManagerGetActiveLevel(mgr);
    const char* levelName = active ? active->name : "(none)";

    if (LevelManagerIsTransitioning(mgr))
    {
        DrawText(TextFormat("Level: %s | Transition: %s %.0f%%",
            levelName, LevelManagerGetStateName(mgr),
            LevelManagerGetProgress(mgr) * 100.0f),
            300, 2, 14, ORANGE);
    }
    else
    {
        DrawText(TextFormat("Level: %s", levelName), 300, 2, 14, YELLOW);
    }

    float px = 10, py = 25;
    float pw = 280, ph = 310;

    for (int i = 0; i < DEBUG_MAX_PANELS; i++)
    {
        if (panels[i].active && panels[i].drawFn)
        {
            DrawRectangle((int)px, (int)py, (int)pw, (int)ph,
                (Color) { 0, 0, 0, 180 });
            panels[i].drawFn(px, py, pw, ph);
            px += pw + 10;
            if (px + pw > (float)GetScreenWidth())
            {
                px = 10;
                py += ph + 10;
            }
        }
    }
}

void DebugSetPerfStats(const DebugPerfStats* stats)
{
    perf = *stats;
}

bool DebugIsVisible(void)
{
    return visible;
}

#endif /* DEBUG_ENABLED */
