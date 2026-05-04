#include "game.h"
#include "arena.h"
#include "debug.h"
#include "raylib.h"
#include "raymath.h"

#include <string.h>
#include <assert.h>
#include <math.h>

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

/* ── Cel Shader Setup (game-side glue) ───────────────────────────────────── */

static bool loadCelShader(Game* game)
{
    const char* vs = "assets/shaders/cel.vs";
    const char* fs = "assets/shaders/cel.fs";

    if (!FileExists(vs) || !FileExists(fs))
    {
        TraceLog(LOG_ERROR, "Game: cel shader files missing (%s / %s)", vs, fs);
        game->celShader.id = 0;
        return false;
    }

    game->celShader = LoadShader(vs, fs);
    if (game->celShader.id == 0)
    {
        TraceLog(LOG_ERROR, "Game: LoadShader returned invalid id");
        return false;
    }

    /* Tell raylib where the model matrix uniform lives so it auto-sets it
     * before each DrawModel call. */
    game->celShader.locs[SHADER_LOC_MATRIX_MODEL] =
        GetShaderLocation(game->celShader, "matModel");

    game->celLocLightDir = GetShaderLocation(game->celShader, "lightDir");
    game->celLocAmbient  = GetShaderLocation(game->celShader, "ambient");
    game->celLocNumBands = GetShaderLocation(game->celShader, "numBands");

    if (game->celLocLightDir < 0 || game->celLocAmbient < 0 || game->celLocNumBands < 0)
    {
        TraceLog(LOG_WARNING,
            "Game: cel shader missing uniforms (lightDir=%d ambient=%d numBands=%d) — continuing",
            game->celLocLightDir, game->celLocAmbient, game->celLocNumBands);
        /* Not fatal: SetShaderValue with -1 loc is a no-op in raylib. */
    }

    game->lightDir = Vector3Normalize((Vector3){ 0.5f, 1.0f, 0.3f });
    game->ambient  = 0.2f;
    game->numBands = 3.0f;
    return true;
}

/* Apply per-frame cel shader uniforms. Must run before RendererDraw3D
 * inside BeginMode3D. No-op if cel shader didn't load. */
static void applyCelShaderUniforms(Game* game)
{
    if (game->celShader.id == 0) return;

    float lightDirArr[3] = { game->lightDir.x, game->lightDir.y, game->lightDir.z };
    SetShaderValue(game->celShader, game->celLocLightDir, lightDirArr, SHADER_UNIFORM_VEC3);
    SetShaderValue(game->celShader, game->celLocAmbient,  &game->ambient,  SHADER_UNIFORM_FLOAT);
    SetShaderValue(game->celShader, game->celLocNumBands, &game->numBands, SHADER_UNIFORM_FLOAT);
}

/* ── Blob Shadow Setup (game-side glue) ──────────────────────────────────── */

#define SHADOW_TEX_SIZE   64
#define SHADOW_MAX_HEIGHT 6.0f
#define SHADOW_GROUND_Y   0.01f

static bool loadShadowResources(Game* game)
{
    Image img = GenImageColor(SHADOW_TEX_SIZE, SHADOW_TEX_SIZE, BLANK);
    Color* pixels = (Color*)img.data;
    float half = SHADOW_TEX_SIZE * 0.5f;

    for (int y = 0; y < SHADOW_TEX_SIZE; y++)
    {
        for (int x = 0; x < SHADOW_TEX_SIZE; x++)
        {
            float dx = (x + 0.5f - half) / half;
            float dy = (y + 0.5f - half) / half;
            float dist = sqrtf(dx * dx + dy * dy);

            float alpha = 1.0f - dist;
            if (alpha < 0.0f) alpha = 0.0f;
            alpha *= alpha;

            unsigned char a = (unsigned char)(alpha * 160.0f);
            pixels[y * SHADOW_TEX_SIZE + x] = (Color){ 0, 0, 0, a };
        }
    }

    game->shadowTex = LoadTextureFromImage(img);
    UnloadImage(img);
    if (game->shadowTex.id == 0)
    {
        TraceLog(LOG_WARNING, "Game: shadow texture upload failed");
        return false;
    }

    Mesh planeMesh = GenMeshPlane(1.0f, 1.0f, 1, 1);
    game->shadowPlane = LoadModelFromMesh(planeMesh);
    if (game->shadowPlane.meshCount == 0)
    {
        TraceLog(LOG_WARNING, "Game: shadow plane creation failed");
        UnloadTexture(game->shadowTex);
        game->shadowTex.id = 0;
        return false;
    }

    game->shadowPlane.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = game->shadowTex;
    return true;
}

/* Walk the renderer's draw list (visible + interpolated) and draw a blob
 * shadow plane under any entry tagged in game->blobOn. Call inside
 * BeginMode3D, after RendererDraw3D. */
static void drawBlobShadows(Game* game)
{
    if (game->shadowTex.id == 0) return;  /* Resources never loaded. */

    BeginBlendMode(BLEND_ALPHA);

    for (int d = 0; d < game->renderer.drawCount; d++)
    {
        DrawEntry* entry = &game->renderer.drawList[d];
        int idx = entry->index;
        if (!game->blobOn[idx]) continue;

        float wx = entry->transform.m12;
        float wy = entry->transform.m13;
        float wz = entry->transform.m14;

        float height = wy;
        if (height < 0.0f) height = 0.0f;
        if (height > SHADOW_MAX_HEIGHT) continue;

        float heightRatio = height / SHADOW_MAX_HEIGHT;
        float scale = game->blobRadius[idx] * 2.0f * (1.0f - heightRatio * 0.5f);
        unsigned char alpha = (unsigned char)(255.0f * (1.0f - heightRatio));

        game->shadowPlane.transform = MatrixMultiply(
            MatrixScale(scale, 1.0f, scale),
            MatrixTranslate(wx, SHADOW_GROUND_Y, wz)
        );

        DrawModel(game->shadowPlane, (Vector3) { 0, 0, 0 }, 1.0f,
            (Color) { 255, 255, 255, alpha });
    }

    EndBlendMode();
}

/* ── Frametime Statistics ────────────────────────────────────────────────── */

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

    /* Game-side render glue. Both helpers are best-effort: they log on
     * failure but don't abort init. drawBlobShadows / applyCelShader
     * become no-ops if resources didn't load. */
    if (!loadCelShader(game))
    {
        TraceLog(LOG_ERROR, "Game: continuing without cel shader (models will use raylib default)");
    }
    if (!loadShadowResources(game))
    {
        TraceLog(LOG_WARNING, "Game: continuing without blob shadows");
    }

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

    /* Guard each unload: resources may have failed to load. shadowPlane
     * owns shadowTex via its material, so UnloadModel frees both. */
    if (game->shadowTex.id != 0) UnloadModel(game->shadowPlane);
    if (game->celShader.id != 0) UnloadShader(game->celShader);

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

                applyCelShaderUniforms(game);
                RendererDraw3D(&game->renderer);
                drawBlobShadows(game);
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

/* ── Render Helpers ──────────────────────────────────────────────────────── */

void GameApplyDefaultShader(Game* game, Model* model)
{
    assert(game && model);
    if (game->celShader.id == 0) return;  /* Shader didn't load. */

    for (int m = 0; m < model->materialCount; m++)
    {
        model->materials[m].shader = game->celShader;
    }
}

void GameSetBlobShadow(Game* game, RenderHandle handle, bool enabled, float radius)
{
    assert(game);
    if (handle < 0 || handle >= RENDERER_MAX_RENDERABLES) return;
    game->blobOn[handle]     = enabled;
    game->blobRadius[handle] = radius;
}

void GameSetClearColor(Game* game, Color color)
{
    assert(game);
    game->clearColor = color;
}
