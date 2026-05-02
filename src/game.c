#include "game.h"
#include "arena.h"
#include "resource.h"
#include "debug.h"
#include "raylib.h"

#include <string.h>
#include <assert.h>

/* ═══════════════════════════════════════════════════════════════════════════
 *  Debug Callbacks
 * ═══════════════════════════════════════════════════════════════════════════ */
 
/* Wrapper so PhysicsDebugDraw can be registered as a DebugRender3DFn */
#ifdef DEBUG_ENABLED
static void physDebugDraw3D(Game* game)
{
    PhysicsDebugDraw(&game->physWorld);
}
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 *  Frametime Statistics
 * ═══════════════════════════════════════════════════════════════════════════ */

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
        .frametimeMs = game->frametimeMs,
        .frametimeAvg = game->frametimeAvg,
        .frametimeMin = game->frametimeMin,
        .frametimeMax = game->frametimeMax,
        .fps = GetFPS(),
        .ticksThisFrame = game->updateCount,
        .alpha = game->alpha,
        .arenaPermanentTotal = game->permanent.arena.size,
        .arenaPermanentFree = ArenaGetFreeMemory(game->permanent),
        .arenaLevelTotal = game->level.arena.size,
        .arenaLevelFree = ArenaGetFreeMemory(game->level),
        .arenaScratchTotal = game->scratch.arena.size,
        .arenaScratchFree = ArenaGetFreeMemory(game->scratch),
        .renderableCount = game->renderer.renderableCount,
        .drawCount = game->renderer.drawCount,
        .statsDrawn = game->renderer.statsDrawn,
        .statsCulled = game->renderer.statsCulled,
        .colliderCount = game->physWorld.colliderCount,
        .pairsChecked = game->physWorld.statsPairsChecked,
        .contactsFound = game->physWorld.statsContactsFound,
        .triggersFound = game->physWorld.statsTriggersFound,
    };
    DebugSetPerfStats(&stats);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Public API
 * ═══════════════════════════════════════════════════════════════════════════ */

bool GameInit(Game* game, Level* initialLevel)
{
    assert(game);
    memset(game, 0, sizeof(*game));

    /* Create memory arenas (rmem MemPools) */
    game->permanent = ArenaCreate(ARENA_PERMANENT_SIZE);
    game->level = ArenaCreate(ARENA_LEVEL_SIZE);
    game->scratch = ArenaCreate(ARENA_SCRATCH_SIZE);

    if (game->permanent.arena.mem == 0 ||
        game->level.arena.mem == 0 ||
        game->scratch.arena.mem == 0)
    {
        TraceLog(LOG_ERROR, "Game: Failed to create memory arenas");
        return false;
    }

    /* Window and audio */
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Prototype Horde");
    if (!IsWindowReady())
    {
        TraceLog(LOG_ERROR, "Failed to initialize window");
        return false;
    }

    SetWindowState(FLAG_VSYNC_HINT);
    SetTargetFPS(RENDER_FPS);
    InitAudioDevice();

    /* Subsystems */
    ResourceInit();
    DebugInit();
    RendererInit(&game->renderer);
    CameraInit(&game->camera);
    PhysicsInit(&game->physWorld);
    LevelManagerInit(&game->levelMgr, game, initialLevel);

    /* Register physics debug draw (slot 2 = F4) */
    DebugRegister3D(2, physDebugDraw3D);

    /* Timing */
    game->accumulator = 0.0f;
    game->frametimeMin = 9999.0f;
    game->frametimeResetTimer = 1.0;
    game->running = true;

    TraceLog(LOG_INFO, "Game: Initialized");
    return true;
}

void GameShutdown(Game* game)
{
    assert(game);

    LevelManagerShutdown(&game->levelMgr, game);
    RendererShutdown(&game->renderer);
    DebugShutdown();
    ResourceShutdown();

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
        /* --- Timing --------------------------------------------------- */

        /* ── Frame timing with spiral-of-death protection ─────────── */
        float frameTime = GetFrameTime();
        if (frameTime > MAX_DELTA_TIME)
        {
            frameTime = MAX_DELTA_TIME;
        }

        updateFrametimeStats(game, frameTime * 1000.0f);

        /* --- Transition state machine (runs every frame) -------------- */
        LevelManagerUpdate(&game->levelMgr, game, frameTime);

        /* --- Fixed-timestep accumulator ------------------------------- */
        game->accumulator += frameTime;
        game->updateCount = 0;

        while (game->accumulator >= FIXED_TIMESTEP)
        {
            /* Save previous transforms for interpolation */
            RendererPreUpdate(&game->renderer);

            /* Input and logic at fixed rate */
            LevelManagerProcessInput(&game->levelMgr, game);
            LevelManagerUpdateLevel(&game->levelMgr, game, FIXED_TIMESTEP);

            /* Physics: process triggers and passive collisions.
             * Gameplay already called MoveAndCollide during its Update,
             * so this handles remaining interactions (trigger overlaps). */
            PhysicsUpdate(&game->physWorld, NULL, NULL);
            
            game->accumulator -= FIXED_TIMESTEP;
            game->updateCount++;

            if (game->updateCount >= MAX_UPDATES_PER_FRAME)
            {
                game->accumulator = 0.0f;  /* Drop remaining time */
                break;
            }
        }

        /* --- Interpolation alpha -------------------------------------- */
        game->alpha = game->accumulator / FIXED_TIMESTEP;

        /* --- Reset scratch arena (per-frame temporaries) -------------- */
        ArenaReset(&game->scratch);

        /* --- Camera update (per visual frame, not per tick) ----------- */
        /*
         * The camera smooths its position using frameTime (visual dt).
         * Levels set the target position during their Update (fixed tick),
         * so by this point the target is up to date.
         * After updating, push the Camera3D to the renderer so
         * BuildDrawList and BeginMode3D use the latest camera.
         */
        CameraUpdate(&game->camera, frameTime);
        RendererSetCamera(&game->renderer, game->camera.camera);

        /* --- Render --------------------------------------------------- */
        /*
         * The game loop is the "director" of the render sequence.
         * Each subsystem contributes its content, but the game loop
         * controls the order and the BeginMode3D/EndMode3D boundaries.
         *
         *   1. ClearBackground        — scene background
         *   2. BuildDrawList           — prepare renderer (culling, sorting)
         *   3. BeginMode3D             — enter 3D context
         *      a. RendererDraw3D      — registered renderables
         *      b. Level->Render3D      — level-specific 3D (grid, decor)
         *      c. DebugRender3D        — 3D gizmos (collider wireframes, etc.)
         *   4. EndMode3D               — exit 3D context
         *   5. Level->RenderHUD        — level 2D overlay (health, ammo, etc.)
         *   6. DEBUG panels            — debug 2D overlay
         *   7. Transition overlay      — drawn last, covers everything
         */
        BeginDrawing();

            ClearBackground(game->renderer.clearColor);

            /* Prepare draw list (interpolation, culling, sorting) */
            RendererBuildDrawList(&game->renderer, game->alpha);

            /* ── 3D phase ── */
            BeginMode3D(game->renderer.camera);

                RendererDraw3D(&game->renderer);
                LevelManagerRender3D(&game->levelMgr, game, game->alpha);
                DebugRender3D(game);

            EndMode3D();

            /* ── 2D phase ── */
            LevelManagerRenderHUD(&game->levelMgr, game, game->alpha);

            DebugUpdate(game);
            feedDebugStats(game);
            DebugRender(game);

            /* Transition overlay (drawn last, covers everything) */
            LevelManagerRender(&game->levelMgr);

        EndDrawing();
    }
}