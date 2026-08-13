#pragma once

/*
 * level_manager.h — Level vtable + transition state machine.
 *
 * DEPENDENCIES: raylib.h
 * OVERRIDES: none
 *
 * Levels are structs of function pointers. The manager runs a small
 * state machine to swap between them with a fullscreen visual effect.
 *
 * The manager carries a `void* user` set at init; every level callback
 * receives it. This keeps the module agnostic of any host type (Game,
 * App, Editor — whatever the embedder uses). The user pointer is also
 * passed to the optional `onSwap` callback so the host can clean up
 * (e.g. reset a memory arena) at the apex of a transition.
 */

#include "raylib.h"
#include <stdbool.h>

/* ── Level vtable ────────────────────────────────────────────────────────── */

typedef Camera3D* (*GetCameraFn)(void* user);
typedef void (*LevelInitFn)(void* user);
typedef void (*LevelShutdownFn)(void* user);
typedef void (*LevelInputFn)(void* user);
typedef void (*LevelUpdateFn)(void* user, float dt);
typedef void (*LevelRender3DFn)(void* user, float alpha);
typedef void (*LevelRenderHUDFn)(void* user, float alpha);

typedef struct
{
    const char* name;

    GetCameraFn      GetCamera;

    LevelInitFn      Init;
    LevelShutdownFn  Shutdown;
    LevelInputFn     ProcessInput;
    LevelUpdateFn    Update;
    LevelRender3DFn  Render3D;
    LevelRenderHUDFn RenderHUD;
} Level;

/* ── Transition state machine ────────────────────────────────────────────── */

typedef enum
{
    TRANSITION_IDLE,
    TRANSITION_FADING_OUT,
    TRANSITION_FADING_IN,
} TransitionState;

/* A transition effect draws a fullscreen overlay based on progress [0,1]. */
typedef void (*TransitionEffectFn)(float progress);

/* Optional cleanup callback fired at the apex of fade-out, after the old
 * level's Shutdown and before the new level's Init. */
typedef void (*LevelSwapFn)(void* user);

typedef struct
{
    Level* activeLevel;
    Level* pendingLevel;

    TransitionState    state;
    TransitionEffectFn effectOut;
    TransitionEffectFn effectIn;
    float              duration;
    float              progress;       /* 0→1 (out), 1→0 (in) */

    void*       user;     /* Passed to every level callback and to onSwap. */
    LevelSwapFn onSwap;   /* Optional; may be NULL.                         */
} LevelManager;

/* ── Built-in Transition Effects ─────────────────────────────────────────── */

void TransitionFade(float progress);
void TransitionWipeLeft(float progress);
void TransitionWipeRight(float progress);

/* ── Default Configuration ───────────────────────────────────────────────── */

#define TRANSITION_DEFAULT_DURATION   0.4f
#define TRANSITION_DEFAULT_EFFECT_OUT TransitionFade
#define TRANSITION_DEFAULT_EFFECT_IN  NULL    /* NULL = reverse effectOut */

/* ── Lifecycle ───────────────────────────────────────────────────────────── */

void LevelManagerInit(LevelManager* mgr, void* user, Level* initialLevel);
void LevelManagerShutdown(LevelManager* mgr);
void LevelManagerUpdate(LevelManager* mgr, float dt);
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

/* ── Level Callbacks (delegated to active level using stored user) ───────── */

void LevelManagerProcessInput(LevelManager* mgr);
void LevelManagerUpdateLevel(LevelManager* mgr, float dt);
void LevelManagerRender3D(LevelManager* mgr, float alpha);
void LevelManagerRenderHUD(LevelManager* mgr, float alpha);
