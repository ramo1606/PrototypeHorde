#include "debug.h"

#ifdef DEBUG_ENABLED

#include "game.h"
#include "level_manager.h"
#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <string.h>

#define FPS_HISTORY_SIZE 120    /* 2 seconds at 60fps */

/* ── State ───────────────────────────────────────────────────────────────── */

static bool visible = false;

static struct 
{
    const char* name;
    DebugPanelDrawFn drawFn;
    DebugRender3DFn  render3DFn;
    bool             active;
} panels[DEBUG_MAX_PANELS];

static DebugPerfStats perf;

/* FPS graph history */
static float fpsHistory[FPS_HISTORY_SIZE];
static int   fpsIndex = 0;

/* ═══════════════════════════════════════════════════════════════════════════
 *  FPS Graph
 * ═══════════════════════════════════════════════════════════════════════════ */

static void DrawFPSGraph(int x, int y, int w, int h)
{
    DrawRectangle(x, y, w, h, (Color) { 0, 0, 0, 160 });
    DrawRectangleLines(x, y, w, h, (Color) { 80, 80, 80, 200 });

    /* Target line */
    float targetRatio = (float)RENDER_FPS / (float)(RENDER_FPS + 20);
    int targetY = y + h - (int)(targetRatio * h);
    DrawLine(x, targetY, x + w, targetY, (Color) { 100, 100, 100, 150 });

    /* Bars */
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
        if (fps >= RENDER_FPS * 0.95f)     color = GREEN;
        else if (fps >= RENDER_FPS * 0.75f) color = YELLOW;
        else                                  color = RED;

        DrawRectangle(barX, barY, (int)barW + 1, barH, color);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Built-in: Performance Panel (F2)
 * ═══════════════════════════════════════════════════════════════════════════ */

static void DrawPerfPanel(float x, float y, float w, float h)
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

    /* FPS graph */
    DrawFPSGraph((int)(x + pad), (int)row, (int)(w - 2 * pad), 50);
    row += 56;

    /* Arena bars */
    size_t permUsed = perf.arenaPermanentTotal - perf.arenaPermanentFree;
    size_t levelUsed = perf.arenaLevelTotal - perf.arenaLevelFree;
    size_t scrUsed = perf.arenaScratchTotal - perf.arenaScratchFree;
    float bw = w - 2 * pad - 80;

    GuiLabel((Rectangle) { x + pad, row, w - 2 * pad, 14 }, "Permanent:");
    row += 14;
    float pv = (perf.arenaPermanentTotal > 0) ? (float)permUsed / (float)perf.arenaPermanentTotal : 0;
    GuiProgressBar((Rectangle) { x + pad, row, bw, 14 }, NULL,
        TextFormat("%zuK/%zuK", permUsed / 1024, perf.arenaPermanentTotal / 1024),
        & pv, 0, 1);
    row += 20;

    GuiLabel((Rectangle) { x + pad, row, w - 2 * pad, 14 }, "Level:");
    row += 14;
    float lv = (perf.arenaLevelTotal > 0) ? (float)levelUsed / (float)perf.arenaLevelTotal : 0;
    GuiProgressBar((Rectangle) { x + pad, row, bw, 14 }, NULL,
        TextFormat("%zuK/%zuK", levelUsed / 1024, perf.arenaLevelTotal / 1024),
        & lv, 0, 1);
    row += 20;

    GuiLabel((Rectangle) { x + pad, row, w - 2 * pad, 14 }, "Scratch:");
    row += 14;
    float sv = (perf.arenaScratchTotal > 0) ? (float)scrUsed / (float)perf.arenaScratchTotal : 0;
    GuiProgressBar((Rectangle) { x + pad, row, bw, 14 }, NULL,
        TextFormat("%zuK/%zuK", scrUsed / 1024, perf.arenaScratchTotal / 1024),
        & sv, 0, 1);
    row += 20;

    /* Transition info */
    /* (will be filled by game.c feeding the stats) */
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Public API
 * ═══════════════════════════════════════════════════════════════════════════ */

void DEBUG_Init(void)
{
    memset(panels, 0, sizeof(panels));
    memset(fpsHistory, 0, sizeof(fpsHistory));
    fpsIndex = 0;
    visible = false;

    DEBUG_RegisterPanel(0, "Performance", DrawPerfPanel);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 13);
}

void DEBUG_Shutdown(void)
{
    /* Nothing to clean up */
}

void DEBUG_RegisterPanel(int slot, const char* name, DebugPanelDrawFn draw_fn)
{
    if (slot < 0 || slot >= DEBUG_MAX_PANELS) return;
    panels[slot].name = name;
    panels[slot].drawFn = draw_fn;
    panels[slot].active = false;
}

void DEBUG_Register3D(int slot, DebugRender3DFn render_fn)
{
    if (slot < 0 || slot >= DEBUG_MAX_PANELS) return;
    panels[slot].render3DFn = render_fn;
}

void DEBUG_Update(Game* game)
{
    (void)game;

    /* Toggle visibility */
    if (IsKeyPressed(KEY_F1)) visible = !visible;

    /* Update FPS history */
    fpsHistory[fpsIndex] = (float)GetFPS();
    fpsIndex = (fpsIndex + 1) % FPS_HISTORY_SIZE;

    if (!visible) return;

    /* Toggle individual panels: F2-F7 */
    if (IsKeyPressed(KEY_F2)) panels[0].active = !panels[0].active;
    if (IsKeyPressed(KEY_F3)) panels[1].active = !panels[1].active;
    if (IsKeyPressed(KEY_F4)) panels[2].active = !panels[2].active;
    if (IsKeyPressed(KEY_F5)) panels[3].active = !panels[3].active;
    if (IsKeyPressed(KEY_F6)) panels[4].active = !panels[4].active;
    if (IsKeyPressed(KEY_F7)) panels[5].active = !panels[5].active;
}

void DEBUG_Render3D(Game* game)
{
    if (!visible) return;

    for (int i = 0; i < DEBUG_MAX_PANELS; i++) 
    {
        if (panels[i].active && panels[i].render3DFn)
            panels[i].render3DFn(game);
    }
}

void DEBUG_Render(Game* game)
{
    (void)game;

    if (!visible) 
    {
        DrawText("F1: Debug", GetScreenWidth() - 90, 10, 14,
            (Color) {
            100, 100, 100, 150
        });
        return;
    }

    /* Hint bar */
    DrawRectangle(0, 0, GetScreenWidth(), 18, (Color) { 0, 0, 0, 160 });
    DrawText("DEBUG | F1 hide | F2-F7 panels", 10, 2, 14, GREEN);

    /* Level & transition info on the hint bar */
    /* (cheap one-liner, always visible when debug is on) */
    LevelManager* mgr = &((Game*)game)->levelMgr;
    Level* active = LEVEL_MGR_GetActiveLevel(mgr);
    const char* levelName = active ? active->name : "(none)";

    if (LEVEL_MGR_IsTransitioning(mgr)) 
    {
        DrawText(TextFormat("Level: %s | Transition: %s %.0f%%",
            levelName, LEVEL_MGR_GetStateName(mgr),
            LEVEL_MGR_GetProgress(mgr) * 100.0f),
            300, 2, 14, ORANGE);
    }
    else 
    {
        DrawText(TextFormat("Level: %s", levelName), 300, 2, 14, YELLOW);
    }

    /* Draw active panels in a row */
    float px = 10, py = 25;
    float pw = 280, ph = 310;

    for (int i = 0; i < DEBUG_MAX_PANELS; i++) 
    {
        if (panels[i].active && panels[i].drawFn) 
        {
            DrawRectangle((int)px, (int)py, (int)pw, (int)ph,
                (Color) {
                0, 0, 0, 180
            });
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

void DEBUG_SetPerfStats(const DebugPerfStats* stats)
{
    perf = *stats;
}

bool DEBUG_IsVisible(void)
{
    return visible;
}

#endif /* DEBUG_ENABLED */
