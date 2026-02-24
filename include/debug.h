#pragma once

#include <stdbool.h>

typedef struct Game Game;
typedef struct DebugTool DebugTool;

struct DebugTool
{
    const char* name;
    int key;
    bool enabled;

    void (*Update)(Game* game);
    void (*RenderOverlay)(Game* game, int* y);  /* 2D panel drawing */
    void (*Render3D)(Game* game);               /* 3D gizmo drawing */
};


void DEBUG_Init(void);
void DEBUG_Shutdown(void);

void DEBUG_RegisterTool(DebugTool tool);
void DEBUG_UnregisterTool(const char* name);

void DEBUG_Update(Game* game);
void DEBUG_Render(Game* game);
void DEBUG_Render3D(Game* game);

bool DEBUG_IsMasterVisible(void);
void DEBUG_SetMasterVisible(bool visible);
bool DEBUG_IsToolEnabled(const char* name);
void DEBUG_SetToolEnabled(const char* name, bool enabled);

int DEBUG_GetToolCount(void);
DebugTool* DEBUG_GetToolByIndex(int index);