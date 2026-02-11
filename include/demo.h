#pragma once

typedef struct Game Game;

void DEMO_Init(Game *game);
void DEMO_Shutdown(Game *game);

void DEMO_ProcessInput(Game *game);
void DEMO_Render3D(Game *game);
int DEMO_RenderHUD(Game *game, int y);