#pragma once

typedef struct Game Game;

typedef void (*LevelInitFn)(Game* game);
typedef void (*LevelShutdownFn)(Game* game);
typedef void (*LevelInputFn)(Game* game);
typedef void (*LevelUpdateFn)(Game* game, float dt);
typedef void (*LevelRender3DFn)(Game* game, float alpha);
typedef void (*LevelRenderHUDFn)(Game* game, float alpha);

typedef struct Level 
{
    const char* name;

    LevelInitFn Init;
    LevelShutdownFn Shutdown;
    LevelInputFn ProcessInput;
    LevelUpdateFn Update;
    LevelRender3DFn Render3D;
    LevelRenderHUDFn RenderHUD;
} Level;