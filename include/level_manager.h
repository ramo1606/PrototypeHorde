#pragma once

#include "level_manager_types.h"
#include <stdbool.h>

typedef struct Game Game;

/* ── Built-in Transition Effects ─────────────────────────────────────────── */

void TransitionFade(float progress);
void TransitionWipeLeft(float progress);
void TransitionWipeRight(float progress);

/* ── Default Configuration ───────────────────────────────────────────────── */

#define TRANSITION_DEFAULT_DURATION   0.4f
#define TRANSITION_DEFAULT_EFFECT_OUT TransitionFade
#define TRANSITION_DEFAULT_EFFECT_IN  NULL    /* NULL = reverse effectOut */

/* ── Lifecycle ───────────────────────────────────────────────────────────── */

void LevelManagerInit(LevelManager* mgr, Game* game, Level* initialLevel);
void LevelManagerShutdown(LevelManager* mgr, Game* game);
void LevelManagerUpdate(LevelManager* mgr, Game* game, float dt);
void LevelManagerRender(const LevelManager* mgr);

/* ── Transition Control ──────────────────────────────────────────────────── */

void LevelManagerTransitionTo(LevelManager* mgr, Level* level,
                              TransitionEffectFn effectOut,
                              TransitionEffectFn effectIn,
                              float duration);

/* Convenience: transition with default fade effect. */
void LevelManagerSwitchTo(LevelManager* mgr, Level* level);

/* ── Queries ─────────────────────────────────────────────────────────────── */

bool        LevelManagerIsTransitioning(const LevelManager* mgr);
Level*      LevelManagerGetActiveLevel(const LevelManager* mgr);
const char* LevelManagerGetStateName(const LevelManager* mgr);
float       LevelManagerGetProgress(const LevelManager* mgr);

/* ── Level Callbacks (delegated to active level) ─────────────────────────── */

void LevelManagerProcessInput(LevelManager* mgr, Game* game);
void LevelManagerUpdateLevel(LevelManager* mgr, Game* game, float dt);
void LevelManagerRender3D(LevelManager* mgr, Game* game, float alpha);
void LevelManagerRenderHUD(LevelManager* mgr, Game* game, float alpha);
