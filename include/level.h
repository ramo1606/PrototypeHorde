#pragma once

typedef struct Game Game;

typedef void (*LevelInitFn)(Game* game);
typedef void (*LevelShutdownFn)(Game* game);
typedef void (*LevelInputFn)(Game* game);
typedef void (*LevelRender3DFn)(Game* game);
typedef int  (*LevelRenderHUDFn)(Game* game, int y);

typedef struct Level 
{
    const char* name;

    LevelInitFn Init;
    LevelShutdownFn Shutdown;
    LevelInputFn ProcessInput;
    LevelRender3DFn Render3D;
    LevelRenderHUDFn RenderHUD;
} Level;