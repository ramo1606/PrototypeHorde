#pragma once

#include "game_types.h"
#include "camera.h"

/* ── Lifecycle ───────────────────────────────────────────────────────────── */

bool GameInit(Game* game, Level* initialLevel);
void GameShutdown(Game* game);
void GameRun(Game* game);

/* ── Render Helpers (game-specific glue) ─────────────────────────────────── */

/* Assign the project's default cel shader to every material of `model`.
 * Call before RendererRegister if you want cel shading on this model. */
void GameApplyDefaultShader(Game* game, Model* model);

/* Toggle a blob shadow under a registered renderable. */
void GameSetBlobShadow(Game* game, RenderHandle handle, bool enabled, float radius);

/* Set the background clear color used each frame. */
void GameSetClearColor(Game* game, Color color);
