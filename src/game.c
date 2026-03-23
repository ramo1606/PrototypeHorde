#include "game.h"
#include "resource_manager.h"
#include "debug.h"
#include "raylib.h"

#include <string.h>
#include <assert.h>

/* ═══════════════════════════════════════════════════════════════════════════
 *  Frametime Statistics
 * ═══════════════════════════════════════════════════════════════════════════ */

static void UpdateFrametimeStats(Game* game, float dtMs)
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

static void FeedDebugStats(Game* game)
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
        .arenaPermanentFree = ARENA_GetFreeMemory(game->permanent),
        .arenaLevelTotal = game->level.arena.size,
        .arenaLevelFree = ARENA_GetFreeMemory(game->level),
        .arenaScratchTotal = game->scratch.arena.size,
        .arenaScratchFree = ARENA_GetFreeMemory(game->scratch),
    };
    DEBUG_SetPerfStats(&stats);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Public API
 * ═══════════════════════════════════════════════════════════════════════════ */

bool GAME_Init(Game* game, Level* initialLevel)
{
    assert(game);
    memset(game, 0, sizeof(*game));

    /* Create memory arenas (rmem MemPools) */
    game->permanent = ARENA_Create(ARENA_PERMANENT_SIZE);
    game->level = ARENA_Create(ARENA_LEVEL_SIZE);
    game->scratch = ARENA_Create(ARENA_SCRATCH_SIZE);

    if (game->permanent.arena.mem == 0 ||
        game->level.arena.mem == 0 ||
        game->scratch.arena.mem == 0)
    {
        TraceLog(LOG_ERROR, "GAME: Failed to create memory arenas");
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
    RESOURCE_Init();
    DEBUG_Init();
    RENDERER_Init(&game->renderer);
    LEVEL_MGR_Init(&game->levelMgr, game, initialLevel);

    /* Timing */
    game->accumulator = 0.0f;
    game->frametimeMin = 9999.0f;
    game->frametimeResetTimer = 1.0;
    game->running = true;

    TraceLog(LOG_INFO, "GAME: Initialized");
    return true;
}

void GAME_Shutdown(Game* game)
{
    assert(game);

    LEVEL_MGR_Shutdown(&game->levelMgr, game);
    RENDERER_Shutdown(&game->renderer);
    DEBUG_Shutdown();
    RESOURCE_Shutdown();

    CloseAudioDevice();
    CloseWindow();

    ARENA_Destroy(&game->scratch);
    ARENA_Destroy(&game->level);
    ARENA_Destroy(&game->permanent);

    game->running = false;
    TraceLog(LOG_INFO, "GAME: Shutdown complete");
}

void GAME_Run(Game* game)
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

        UpdateFrametimeStats(game, frameTime * 1000.0f);

        /* --- Transition state machine (runs every frame) -------------- */
        LEVEL_MGR_Update(&game->levelMgr, game, frameTime);

        /* --- Fixed-timestep accumulator ------------------------------- */
        game->accumulator += frameTime;
        game->updateCount = 0;

        while (game->accumulator >= FIXED_TIMESTEP)
        {
            /* Save previous transforms for interpolation */
            RENDERER_PreUpdate(&game->renderer);

            /* Input and logic at fixed rate */
            LEVEL_MGR_ProcessInput(&game->levelMgr, game);
            LEVEL_MGR_UpdateLevel(&game->levelMgr, game, FIXED_TIMESTEP);
            
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
        ARENA_Reset(&game->scratch);

        /* --- Render --------------------------------------------------- */
        /*
         * The game loop is the "director" of the render sequence.
         * Each subsystem contributes its content, but the game loop
         * controls the order and the BeginMode3D/EndMode3D boundaries.
         *
         *   1. ClearBackground        — scene background
         *   2. BuildDrawList           — prepare renderer (culling, sorting)
         *   3. BeginMode3D             — enter 3D context
         *      a. RENDERER_Draw3D      — registered renderables
         *      b. Level->Render3D      — level-specific 3D (grid, decor)
         *      c. DEBUG_Render3D       — 3D gizmos (collider wireframes, etc.)
         *   4. EndMode3D               — exit 3D context
         *   5. Level->RenderHUD        — level 2D overlay (health, ammo, etc.)
         *   6. DEBUG panels            — debug 2D overlay
         *   7. Transition overlay      — drawn last, covers everything
         */
        BeginDrawing();

        ClearBackground(game->renderer.clearColor);

        /* Prepare draw list (interpolation, culling, sorting) */
        RENDERER_BuildDrawList(&game->renderer, game->alpha);

        /* ── 3D phase ── */
        BeginMode3D(game->renderer.camera);

            RENDERER_Draw3D(&game->renderer);
            LEVEL_MGR_Render3D(&game->levelMgr, game, game->alpha);
            DEBUG_Render3D(game);

        EndMode3D();

        /* ── 2D phase ── */
        LEVEL_MGR_RenderHUD(&game->levelMgr, game, game->alpha);

        DEBUG_Update(game);
        FeedDebugStats(game);
        DEBUG_Render(game);

        /* Transition overlay (drawn last, covers everything) */
        LEVEL_MGR_Render(&game->levelMgr);

        EndDrawing();
    }
}