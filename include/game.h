/*******************************************************************************************
*
*   game.h — Game Core (Application Framework)
*
*   Central orchestrator of the engine. The Game struct owns all major subsystems
*   (memory, rendering, physics, level management) and manages the actor lifecycle.
*   It implements the main game loop with fixed-timestep updates decoupled from
*   variable-framerate rendering.
*
*   Architecture:
*       Game
*        ├── MemorySystem      → Pool allocators for actors and components
*        ├── Renderer           → Frustum culling, draw-list sorting, frame output
*        ├── PhysWorld          → Broad-phase collision queries (sweep-and-prune)
*        ├── LevelManager       → Level loading, transitions, lifecycle
*        ├── actors[]           → Active actor list (up to GAME_MAX_ACTORS)
*        └── pendingActors[]    → Actors queued during mid-update (deferred insertion)
*
*   Game Loop (Glenn Fiedler's "Fix Your Timestep!"):
*       1. Accumulate frame time into an accumulator
*       2. Save previous transforms for interpolation
*       3. Drain accumulator in FIXED_TIMESTEP increments (deterministic physics)
*       4. Interpolate transforms with alpha = remainder / FIXED_TIMESTEP
*       5. Render the interpolated state
*       6. Restore transforms so the next physics step starts from true state
*
*   Actor Lifecycle:
*       GAME_AddActor()  → If mid-update, goes to pendingActors[] (deferred)
*                          If not updating, goes directly to actors[]
*       FixedUpdate      → Updates all actors → flushes pending → destroys dead
*       GAME_RemoveActor() → Swap-remove from whichever list contains the actor
*
*   Naming Convention:
*       Enums:   GAME_STATE_*
*       API:     GAME_*
*
********************************************************************************************/
#pragma once
#include "raylib.h"
#include "memory.h"
#include "level_manager.h"
#include "renderer.h"
#include "physics_world.h"
#include <stdbool.h>

typedef struct Actor Actor;
typedef struct Level Level;
typedef struct Game Game;

typedef enum GameState GameState;

#define SCREEN_WIDTH   1024              /* Window width in pixels                            */
#define SCREEN_HEIGHT  768               /* Window height in pixels                           */
#define GAME_TITLE  "Prototype Horde"    /* Window title bar text                             */

/* ── Timing Constants ────────────────────────────────────────────────────── */
/*                                                                             *
 *  UPDATE_RATE:  How many physics/logic ticks per second (Hz).                *
 *  FIXED_TIMESTEP: Duration of one tick = 1/UPDATE_RATE (~16.67ms at 60Hz).   *
 *  MAX_DELTA_TIME: Safety clamp — prevents spiral-of-death when the           *
 *                  application stalls (e.g. breakpoint, window drag).          *
 *                  At 0.25s the engine will lose time rather than try to       *
 *                  catch up with 15+ ticks in a single frame.                 *
 *  RENDER_FPS:  Target rendering framerate (independent from update rate).     *
 *                                                                             */
#define UPDATE_RATE 60
#define FIXED_TIMESTEP (1.0f / (float)UPDATE_RATE)
#define MAX_DELTA_TIME 0.25f
#define RENDER_FPS 30

 /* ── Capacity Limits ─────────────────────────────────────────────────────── */
#define GAME_MAX_ACTORS     512
#define GAME_MAX_PENDING    64

/* ── Game State ──────────────────────────────────────────────────────────── */
enum GameState
{
    GAME_STATE_GAMEPLAY,                 /* Normal gameplay — actors update and receive input  */
    GAME_STATE_PAUSED,                   /* Paused — actors frozen, input still processed      */
    GAME_STATE_QUIT                      /* Quit requested — exits main loop next frame        */
};

struct Game
{
    GameState state;                     /* Current application state                         */

    float accumulator;                   /* Unprocessed time carried between frames (seconds) */
    int updateCount;                     /* Number of fixed-steps executed this frame         */

    /* ── Subsystems ─────────────────────────────────────────────────────── */
    MemorySystem memory;                 /* Pool allocators (ObjPool + MemPool)               */
    LevelManager levelMgr;               /* Level loading, transitions, active level tracking */
    Renderer renderer;                   /* Draw-list builder, frustum culler, frame renderer */
    PhysWorld physWorld;                  /* Spatial queries: overlap, raycast, sweep-and-prune*/

    /* InputSystem  input;   */          /* Phase 1  — planned                                */
    /* EventSystem  events;  */          /* Phase 2  — planned                                */
    /* AudioSystem  audio;   */          /* Phase 8  — planned                                */

    /* ── Actor Management ───────────────────────────────────────────────── */
    Actor* actors[GAME_MAX_ACTORS];      /* Active actor list (unordered for O(1) removal)    */
    int    actorCount;                   /* Number of currently active actors                 */
    Actor* pendingActors[GAME_MAX_PENDING]; /* Deferred queue for actors added mid-update     */
    int    pendingCount;                 /* Number of actors waiting to be promoted           */
    bool updatingActors;                 /* Guard flag: true while iterating actors in update */
    int actorsCreated;                   /* Lifetime counter — total actors ever created      */
};

/* ── Public API ──────────────────────────────────────────────────────────── */
bool GAME_Init(Game* game, Level* initialLevel);
void GAME_Shutdown(Game* game);
void GAME_Run(Game* game);

void GAME_AddActor(Game* game, Actor* actor);
void GAME_RemoveActor(Game* game, Actor* actor);
void GAME_RemoveAllActors(Game* game);

void GAME_ChangeLevel(Game* game, Level* level);

Actor* GAME_FindActorByTag(Game* game, unsigned int tag);
int GAME_FindActorsByTag(Game* game, unsigned int tag, Actor** outArray, int maxResults);

float GAME_GetTime(Game* game);