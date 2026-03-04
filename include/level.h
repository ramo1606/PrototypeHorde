/*******************************************************************************************
*
*   level.h — Level (Static Function Table)
*
*   A Level is a static, zero-allocation description of a game scene. Instead of a class
*   hierarchy or data-driven loaders, each level is a plain struct of function pointers
*   that hook into the game loop at well-defined points (init, shutdown, input, render).
*
*   Architecture:
*       Game
*        └── LevelManager
*               └── activeLevel ──▶ Level { Init, Shutdown, ProcessInput, Render3D, RenderHUD }
*                                       │
*                                       └── Init() spawns Actors via GAME_AddActor()
*
*   Design Rationale:
*       Madhav uses JSON files + a LevelLoader class to deserialize scenes at runtime.
*       Knight uses a Scene class with a full scene graph rooted at the level.
*       We use static function tables instead — the level struct is a compile-time
*       constant, requires no heap allocation, and binds logic directly to code.
*       Levels don't own actors; they spawn them in Init and the Game manages their
*       lifecycle. This keeps levels lightweight and avoids ownership ambiguity.
*
*   Lifecycle:
*       Level.Init()         → Called once when the level becomes active (spawn actors)
*       Level.ProcessInput() → Called every frame for level-specific input handling
*       Level.Render3D()     → Called inside BeginMode3D for level-specific 3D drawing
*       Level.RenderHUD()    → Called after EndMode3D for level-specific 2D overlay
*       Level.Shutdown()     → Called once when leaving the level (cleanup)
*
*   Naming Convention:
*       Typedefs: Level*Fn
*       Struct:   Level
*
********************************************************************************************/
#pragma once

typedef struct Game Game;
typedef struct Level Level;

/* ── Function Pointer Typedefs ──────────────────────────────────────────── */

typedef void (*LevelInitFn)(Game* game);             /* Spawn actors, configure camera        */
typedef void (*LevelShutdownFn)(Game* game);          /* Level-specific cleanup (actors auto-destroyed) */
typedef void (*LevelInputFn)(Game* game);             /* Per-frame level input (key bindings)  */
typedef void (*LevelRender3DFn)(Game* game);          /* Custom 3D drawing (grid, gizmos)     */
typedef int  (*LevelRenderHUDFn)(Game* game, int y);  /* 2D overlay; returns next Y for stacking */

/* ── Level Struct ───────────────────────────────────────────────────────── */

struct Level
{
    const char* name;               /* Display name (shown in debug overlay / logs)   */

    LevelInitFn     Init;           /* Setup callback — NULL if no setup needed        */
    LevelShutdownFn Shutdown;       /* Teardown callback — NULL if no cleanup needed   */
    LevelInputFn    ProcessInput;   /* Input callback — NULL if level has no bindings  */
    LevelRender3DFn Render3D;       /* 3D render callback — NULL if nothing to draw    */
    LevelRenderHUDFn RenderHUD;     /* HUD callback — NULL if no overlay              */
} ;
