#include "game.h"
#include "arena.h"
#include "debug.h"
#include "raylib.h"

#include <assert.h>
#include <string.h>

/* ── Debug Callbacks ─────────────────────────────────────────────────────── */

#ifdef DEBUG_ENABLED
static void physDebugDraw3D(Game* game)
{
    PhysicsDebugDraw(&game->physWorld);
}
#endif

/* ── Level Swap Callback ─────────────────────────────────────────────────── */
/* Fires at the apex of fade-out, after the old level's Shutdown.
 * Releases all level-arena memory at once (frees per-level allocations). */
static void onLevelSwap(void* user)
{
    Game* game = (Game*)user;
    ArenaReset(&game->level);
}

static void updateFrametimeStats(Game* game, float dtMs)
{
    game->frametimeMs = dtMs;

    if (dtMs < game->frametimeMin) game->frametimeMin = dtMs;
    if (dtMs > game->frametimeMax) game->frametimeMax = dtMs;
    game->frametimeAccum += dtMs;
    game->frametimeCount++;

    game->frametimeResetTimer -= (double)dtMs / 1000.0;
    if (game->frametimeResetTimer <= 0.0)
    {
        game->frametimeAvg = (game->frametimeCount > 0)
            ? game->frametimeAccum / (float)game->frametimeCount
            : 0.0f;
        game->frametimeMin = 9999.0f;
        game->frametimeMax = 0.0f;
        game->frametimeAccum = 0.0f;
        game->frametimeCount = 0;
        game->frametimeResetTimer = 1.0;
    }
}

static void feedDebugStats(Game* game)
{
    DebugPerfStats stats =
    {
        .frametimeMs   = game->frametimeMs,
        .frametimeAvg  = game->frametimeAvg,
        .frametimeMin  = game->frametimeMin,
        .frametimeMax  = game->frametimeMax,
        .fps           = GetFPS(),
        .ticksThisFrame = game->updateCount,
        .alpha         = game->alpha,
        .arenaPermanentTotal = game->permanent.arena.size,
        .arenaPermanentFree  = ArenaGetFreeMemory(game->permanent),
        .arenaLevelTotal     = game->level.arena.size,
        .arenaLevelFree      = ArenaGetFreeMemory(game->level),
        .arenaScratchTotal   = game->scratch.arena.size,
        .arenaScratchFree    = ArenaGetFreeMemory(game->scratch),
        .renderableCount = game->renderer.renderableCount,
        .drawCount       = game->renderer.drawCount,
        .statsDrawn      = game->renderer.statsDrawn,
        .statsCulled     = game->renderer.statsCulled,
        .colliderCount  = game->physWorld.colliderCount,
        .pairsChecked   = game->physWorld.statsPairsChecked,
        .contactsFound  = game->physWorld.statsContactsFound,
        .triggersFound  = game->physWorld.statsTriggersFound,
    };
    DebugSetPerfStats(&stats);
}

/* ── Public API ──────────────────────────────────────────────────────────── */

/* Tear down whatever was already brought up. Safe to call at any
 * partial-init point because each undo is gated by a "was it allocated?"
 * test (mem != 0, IsWindowReady, IsAudioDeviceReady). */
static void gameInitCleanup(Game* game)
{
    if (IsAudioDeviceReady()) CloseAudioDevice();
    if (IsWindowReady())      CloseWindow();
    if (game->scratch.arena.mem)   ArenaDestroy(&game->scratch);
    if (game->level.arena.mem)     ArenaDestroy(&game->level);
    if (game->permanent.arena.mem) ArenaDestroy(&game->permanent);
}

bool GameInit(Game* game, Level* initialLevel)
{
    assert(game);
    memset(game, 0, sizeof(*game));

    game->permanent = ArenaCreate(ARENA_PERMANENT_SIZE);
    game->level     = ArenaCreate(ARENA_LEVEL_SIZE);
    game->scratch   = ArenaCreate(ARENA_SCRATCH_SIZE);

    if (game->permanent.arena.mem == 0 ||
        game->level.arena.mem == 0 ||
        game->scratch.arena.mem == 0)
    {
        TraceLog(LOG_ERROR, "Game: Failed to create memory arenas");
        gameInitCleanup(game);
        return false;
    }

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Prototype Horde");
    if (!IsWindowReady())
    {
        TraceLog(LOG_ERROR, "Game: Failed to initialize window");
        gameInitCleanup(game);
        return false;
    }

    SetWindowState(FLAG_VSYNC_HINT);
    SetTargetFPS(RENDER_FPS);

    InitAudioDevice();
    if (!IsAudioDeviceReady())
    {
        TraceLog(LOG_WARNING, "Game: Audio device unavailable, continuing silent");
        /* Not fatal — game can run without audio. */
    }

    game->clearColor = (Color){ 20, 20, 40, 255 };

    DebugInit();
    RendererInit(&game->renderer);
    CameraInit(&game->camera);
    PhysicsInit(&game->physWorld);

    LevelManagerInit(&game->levelMgr, game, initialLevel);
    game->levelMgr.onSwap = onLevelSwap;

    DebugRegister3D(2, physDebugDraw3D);

    game->accumulator         = 0.0f;
    game->frametimeMin        = 9999.0f;
    game->frametimeResetTimer = 1.0;
    game->running             = true;

    TraceLog(LOG_INFO, "Game: Initialized");
    return true;
}

void GameShutdown(Game* game)
{
    assert(game);

    LevelManagerShutdown(&game->levelMgr);

    RendererShutdown(&game->renderer);
    DebugShutdown();

    CloseAudioDevice();
    CloseWindow();

    ArenaDestroy(&game->scratch);
    ArenaDestroy(&game->level);
    ArenaDestroy(&game->permanent);

    game->running = false;
    TraceLog(LOG_INFO, "Game: Shutdown complete");
}

void GameRun(Game* game)
{
    assert(game);

    while (!WindowShouldClose() && game->running)
    {
        float frameTime = GetFrameTime();
        if (frameTime > MAX_DELTA_TIME) frameTime = MAX_DELTA_TIME;

        updateFrametimeStats(game, frameTime * 1000.0f);

        LevelManagerUpdate(&game->levelMgr, frameTime);

        game->accumulator += frameTime;
        game->updateCount = 0;

        while (game->accumulator >= FIXED_TIMESTEP)
        {
            RendererPreUpdate(&game->renderer);

            LevelManagerProcessInput(&game->levelMgr);
            LevelManagerUpdateLevel(&game->levelMgr, FIXED_TIMESTEP);

            PhysicsUpdate(&game->physWorld, NULL, NULL);

            game->accumulator -= FIXED_TIMESTEP;
            game->updateCount++;

            if (game->updateCount >= MAX_UPDATES_PER_FRAME)
            {
                game->accumulator = 0.0f;
                break;
            }
        }

        game->alpha = game->accumulator / FIXED_TIMESTEP;

        ArenaReset(&game->scratch);

        CameraUpdate(&game->camera, frameTime);

        BeginDrawing();

            ClearBackground(game->clearColor);

            RendererBuildDrawList(&game->renderer, game->camera.camera, game->alpha);

            BeginMode3D(game->camera.camera);
                RendererDraw3D(&game->renderer);
                LevelManagerRender3D(&game->levelMgr, game->alpha);
                DebugRender3D(game);

            EndMode3D();

            LevelManagerRenderHUD(&game->levelMgr, game->alpha);

            DebugUpdate(game);
            feedDebugStats(game);
            DebugRender(game);

            LevelManagerRender(&game->levelMgr);

        EndDrawing();
    }
}

void GameSetClearColor(Game* game, Color color)
{
    assert(game);
    game->clearColor = color;
}
