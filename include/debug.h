#pragma once

#include "debug_types.h"
#include <stdbool.h>

#ifdef DEBUG_ENABLED

void DebugInit(void);
void DebugShutdown(void);

/* Register a 2D overlay panel. slot 0..5 maps to F2..F7. */
void DebugRegisterPanel(int slot, const char* name, DebugPanelDrawFn drawFn);

/* Register a 3D gizmo renderer. slot 0..5 maps to F2..F7 (shared toggle). */
void DebugRegister3D(int slot, DebugRender3DFn renderFn);

void DebugUpdate(Game* game);
void DebugRender3D(Game* game);   /* Call inside BeginMode3D */
void DebugRender(Game* game);     /* Call after EndMode3D */

void DebugSetPerfStats(const DebugPerfStats* stats);

bool DebugIsVisible(void);

#else /* release: zero-cost no-ops */

#define DebugInit()
#define DebugShutdown()
#define DebugRegisterPanel(slot, name, fn)
#define DebugRegister3D(slot, fn)
#define DebugUpdate(game)
#define DebugRender3D(game)
#define DebugRender(game)
#define DebugSetPerfStats(stats)
#define DebugIsVisible() false

#endif /* DEBUG_ENABLED */
