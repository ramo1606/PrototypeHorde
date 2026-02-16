#pragma once

typedef struct Game Game;

typedef void (*SceneInitFn)(Game* game);
typedef void (*SceneShutdownFn)(Game* game);
typedef void (*SceneInputFn)(Game* game);
typedef void (*SceneRender3DFn)(Game* game);
typedef int  (*SceneRenderHUDFn)(Game* game, int y);

typedef struct Scene 
{
    const char* name;

    SceneInitFn Init;
    SceneShutdownFn Shutdown;
    SceneInputFn ProcessInput;
    SceneRender3DFn Render3D;
    SceneRenderHUDFn RenderHUD;
} Scene;