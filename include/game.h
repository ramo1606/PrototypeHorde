#pragma once

/*
 * game.h — Top-level game object: owns all engine subsystems and actors.
 *
 * Game is the root of the entire engine hierarchy.  It holds value-typed
 * subsystems (MemorySystem, Renderer, PhysWorld, LevelManager) and the
 * actor flat arrays.  The main loop lives in GAME_Run and uses a fixed-
 * timestep model:
 *
 *   1. Accumulate frame time.
 *   2. Save previous actor positions (for visual interpolation).
 *   3. Run N fixed updates at FIXED_TIMESTEP until accumulator is drained.
 *   4. Interpolate actor positions by alpha = accumulator / FIXED_TIMESTEP.
 *   5. Draw the interpolated frame.
 *   6. Restore physics positions from the saved snapshots.
 *
 * Pending actor queue: actors created during GAME_FixedUpdate are buffered
 * in pendingActors[] to avoid invalidating the main actors[] iteration.
 * They are promoted at the end of each fixed tick.
 *
 * Architecture position:
 *   main() → Game (stack/heap allocation)
 *   Game owns: MemorySystem, LevelManager, Renderer, PhysWorld, actors[]
 */

#include "raylib.h"
#include "memory.h"
#include "level_manager.h"
#include "renderer.h"
#include "physics_world.h"
#include <stdbool.h>

typedef struct Actor Actor;
typedef struct Level Level;
typedef struct Game Game;

/* ── Screen & Timing Constants ──────────────────────────────────── */

#define SCREEN_WIDTH   1024             /* Viewport width in pixels */
#define SCREEN_HEIGHT  768              /* Viewport height in pixels */
#define GAME_TITLE  "Prototype Horde"   /* Window title string */

#define UPDATE_RATE 60                              /* Fixed update frequency in Hz */
#define FIXED_TIMESTEP (1.0f / (float)UPDATE_RATE) /* Duration of one fixed update step in seconds */
#define MAX_DELTA_TIME 0.25f                        /* Frame time is clamped to this to prevent spiral-of-death on slow frames */
#define RENDER_FPS 30                               /* Target rendering frame rate passed to Raylib */

#define GAME_MAX_ACTORS     512  /* Maximum number of actors that can exist simultaneously */
#define GAME_MAX_PENDING    64   /* Maximum number of actors that can be pending promotion in one fixed tick */

/* ── Game State Enum ────────────────────────────────────────────── */

typedef enum
{
    GAME_STATE_GAMEPLAY,  /* Normal play — actors update, input is forwarded */
    GAME_STATE_PAUSED,    /* Game is paused — actors do not update */
	GAME_STATE_QUIT       /* Signals the main loop to exit cleanly */
} GameState;

/* ── Game Struct ────────────────────────────────────────────────── */

struct Game
{
    GameState state;       /* Current lifecycle state of the game */

    float accumulator;     /* Time accumulated since the last fixed update (seconds) */
    int   updateCount;     /* Number of fixed updates executed in the current frame (debug counter) */

    MemorySystem memory;   /* Pool allocators for actors and components */
    LevelManager levelMgr; /* Level state machine (IDLE → FADING_OUT → swap → FADING_IN → IDLE) */
    Renderer     renderer; /* 3D renderer — frustum cull, sort, draw */
    PhysWorld    physWorld;/* Collision world — broadphase + spatial queries */

    /* InputSystem  input;   */         /* Phase 1 */
    /* EventSystem  events;  */         /* Phase 2 */
    /* AudioSystem  audio;   */         /* Phase 8 */

    Actor *actors[GAME_MAX_ACTORS];    /* Flat array of all live actors (unsorted pointer list) */
    int    actorCount;                 /* Number of active entries in actors[] */
    Actor *pendingActors[GAME_MAX_PENDING]; /* Actors created during a fixed update, waiting to be promoted */
    int    pendingCount;               /* Number of entries in pendingActors[] */
    bool   updatingActors;             /* True while actors[] is being iterated — new actors go to pending */
    int    actorsCreated;              /* Running total of actors ever created (debug/diagnostic counter) */
};

/* ── Lifecycle ──────────────────────────────────────────────────── */

bool GAME_Init(Game* game, Level* initialLevel); // Initialise all subsystems, open the window, and load the initial level; returns false on failure
void GAME_Shutdown(Game* game);                  // Shut down all subsystems, destroy all actors, and close the window
void GAME_Run(Game* game);                       // Execute the main loop (fixed-timestep update + interpolated render) until quit is requested

/* ── Actor Management ───────────────────────────────────────────── */

void GAME_AddActor(Game* game, Actor* actor);    // Add an actor to the live list (or pending list if called during an update pass)
void GAME_RemoveActor(Game* game, Actor* actor); // Remove an actor from the live or pending list (swap-remove, O(n))
void GAME_RemoveAllActors(Game* game);           // Destroy every actor in both the live and pending lists

/* ── Level Management ───────────────────────────────────────────── */

void GAME_ChangeLevel(Game* game, Level* level); // Initiate a level transition via the LevelManager (fades out, swaps, fades in)

/* ── Tag Queries ────────────────────────────────────────────────── */

Actor* GAME_FindActorByTag(Game* game, unsigned int tag);                                        // Return the first actor whose tag bitmask includes any bit of tag, or NULL
int    GAME_FindActorsByTag(Game* game, unsigned int tag, Actor** outArray, int maxResults);     // Fill outArray with all actors matching tag; returns count found

/* ── Time ───────────────────────────────────────────────────────── */

float GAME_GetTime(Game* game); // Return the total elapsed time in seconds since the window was opened (wraps Raylib GetTime)