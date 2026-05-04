#pragma once

#include "arena.h"
#include "camera_types.h"
#include "config.h"
#include "physics.h"
#include "renderer.h"
#include "level_manager.h"
#include <stdbool.h>

typedef struct Game
{
    /* Memory arenas (rmem MemPool by value). See docs/modules/arena.md. */
    MemArena permanent;        /* Lives for the entire program            */
    MemArena level;            /* Reset on level transitions              */
    MemArena scratch;          /* Reset every frame                       */

    /* Subsystems */
    LevelManager levelMgr;
    Renderer     renderer;
    PhysWorld    physWorld;
    GameCamera   camera;

    /* Render state owned by the game (no longer in renderer) */
    Color   clearColor;
    Shader  celShader;
    int     celLocLightDir;
    int     celLocAmbient;
    int     celLocNumBands;
    Vector3 lightDir;
    float   ambient;
    float   numBands;

    /* Blob shadows: parallel state indexed by RenderHandle. */
    Model   shadowPlane;
    Texture shadowTex;
    bool    blobOn[RENDERER_MAX_RENDERABLES];
    float   blobRadius[RENDERER_MAX_RENDERABLES];

    /* Timing */
    float accumulator;         /* Unprocessed time carried between frames */
    float alpha;               /* Interpolation factor (0..1)             */
    int   updateCount;         /* Fixed-steps executed this frame         */

    /* Frametime stats (debug overlay) */
    float  frametimeMs;
    float  frametimeMin;
    float  frametimeMax;
    float  frametimeAvg;
    float  frametimeAccum;
    int    frametimeCount;
    double frametimeResetTimer;

    bool running;
} Game;
