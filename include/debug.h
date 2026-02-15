#pragma once

#include <stdbool.h>

typedef struct Game Game;

void DEBUG_Init(void);

void DEBUG_Update(Game* game);
void DEBUG_Render(Game* game);

bool DEBUG_IsVisible(void);
void DEBUG_SetVisible(bool visible);