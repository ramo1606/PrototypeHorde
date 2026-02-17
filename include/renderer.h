#pragma once

#include "raylib.h"

typedef struct Game Game;

typedef struct
{
    Camera3D camera;
    Color    clearColor;
} Renderer;

void RENDERER_Init(Renderer* renderer);
void RENDERER_Shutdown(Renderer* renderer);

void RENDERER_DrawFrame(Renderer* renderer, Game* game);

void RENDERER_SetCamera(Renderer* renderer, Camera3D camera);
void RENDERER_SetClearColor(Renderer* renderer, Color color);