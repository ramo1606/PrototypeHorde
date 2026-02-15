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

    SceneInitFn SCENE_Init;
    SceneShutdownFn SCENE_Shutdown;
    SceneInputFn SCENE_ProcessInput;
    SceneRender3DFn SCENE_Render3D;
    SceneRenderHUDFn SCENE_RenderHUD;
} Scene;