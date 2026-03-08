/*******************************************************************************************
*
*   level_sandbox.c — Development Sandbox Level
*
*   Central playground for testing every engine subsystem. Organized into spatial
*   zones that each exercise different features:
*
*       Zone 1 — Movement:  Open area for testing camera-relative movement
*       Zone 2 — Collision: Walls, ramps, columns for collider/camera wall-clamp tests
*       Zone 3 — Spawning:  Designated area + hotkeys to spawn cubes/spheres
*       Zone 4 — Stress:    Mass-spawn area for performance testing
*
*   Player: Capsule mesh + BoxComponent + CameraTPS (orbit) + PlayerMovement
*
*   Hotkeys:
*       1        — Spawn cube in front of player
*       2        — Spawn sphere in front of player
*       3        — Stress spawn (10 cubes in a grid)
*       X        — Delete all spawned actors (keeps world geometry)
*       F2       — Toggle ground grid
*       F3       — Toggle collider wireframes
*       F4       — Toggle zone labels
*       Tab      — Cycle camera distance (close / medium / far)
*
*   This level grows with each new Phase — add test zones as new systems come online.
*
********************************************************************************************/
#include "level.h"
#include "game.h"
#include "actor.h"
#include "mesh_component.h"
#include "box_component.h"
#include "sphere_component.h"
#include "move_component.h"
#include "camera_tps.h"
#include "player_movement.h"
#include "renderer.h"
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>

/* ═══════════════════════════════════════════════════════════════════════════
 *  Constants
 * ═══════════════════════════════════════════════════════════════════════════ */

#define MAX_SPAWNED         256
#define SPAWN_DISTANCE      3.0f        /* How far in front of player to spawn     */
#define STRESS_SPAWN_COUNT  10
#define STRESS_SPAWN_SPACING 2.5f

 /* Camera distance presets (close / medium / far) */
static const float CAMERA_HORZ_PRESETS[] = { 4.0f, 8.0f, 14.0f };
static const float CAMERA_VERT_PRESETS[] = { 2.5f, 4.0f,  7.0f };
static const int   CAMERA_PRESET_COUNT = 3;

/* Zone centers (used for labels and spawning reference) */
static const Vector3 ZONE_MOVEMENT_CENTER = { 0.0f, 0.0f,   0.0f };
static const Vector3 ZONE_COLLISION_CENTER = { 30.0f, 0.0f,   0.0f };
static const Vector3 ZONE_SPAWN_CENTER = { 0.0f, 0.0f, -30.0f };
static const Vector3 ZONE_STRESS_CENTER = { 30.0f, 0.0f, -30.0f };

/* ═══════════════════════════════════════════════════════════════════════════
 *  Level-Local State
 *
 *  Meshes and materials live here because there's no Asset Manager yet
 *  (Phase 6). On Shutdown we UnloadMesh/UnloadMaterial to avoid leaks.
 *  When Phase 6 arrives, these move to the asset registry.
 * ═══════════════════════════════════════════════════════════════════════════ */

 /* Player references */
static Actor* s_player = NULL;
static CameraTPS* s_camera = NULL;
static PlayerMovement* s_movement = NULL;

/* Meshes — owned by this level, freed on Shutdown */
static Mesh s_meshCapsule;
static Mesh s_meshCube;
static Mesh s_meshSphere;
static Mesh s_meshPlane;
static Mesh s_meshRamp;

/* Materials — owned by this level, freed on Shutdown */
static Material s_matPlayer;
static Material s_matFloor;
static Material s_matWall;
static Material s_matSpawnCube;
static Material s_matSpawnSphere;
static Material s_matRamp;

/* Spawn tracking */
static Actor* s_spawnedActors[MAX_SPAWNED];
static int    s_spawnedCount = 0;

/* Debug toggles */
static bool s_showGrid = true;
static bool s_showColliders = false;
static bool s_showZoneLabels = true;

/* Camera distance preset index */
static int s_cameraPreset = 1;     /* Start at medium */

/* ═══════════════════════════════════════════════════════════════════════════
 *  Helper: Create Colored Material
 * ═══════════════════════════════════════════════════════════════════════════ */

static Material CreateColorMaterial(Color color)
{
    Material mat = LoadMaterialDefault();
    mat.maps[MATERIAL_MAP_DIFFUSE].color = color;
    return mat;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Helper: Spawn Static Geometry Actor
 *
 *  Creates an actor with a MeshComponent and optional BoxComponent.
 *  Used for walls, floors, ramps — anything that doesn't move.
 * ═══════════════════════════════════════════════════════════════════════════ */

static Actor* SpawnStaticActor(Game* game, Mesh* mesh, Material* material,
    Vector3 position, Vector3 rotation, float scale,
    bool addCollider)
{
    Actor* actor = ACTOR_Create(game);
    ACTOR_SetPosition(actor, position);
    ACTOR_SetRotation(actor, rotation);
    ACTOR_SetScale(actor, scale);

    MESH_COMPONENT_Create(actor, mesh, material);

    if (addCollider)
    {
        BoxComponent* bc = BOX_COMPONENT_Create(actor);
        BOX_COMPONENT_SetFromMesh(bc, *mesh);
    }

    return actor;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Helper: Spawn Player
 * ═══════════════════════════════════════════════════════════════════════════ */

 /*------------------------------------------------------------------------------------
  * SpawnPlayer
  *
  *   Creates the playable actor with:
  *     - MeshComponent: capsule-shaped mesh (cylinder for now)
  *     - BoxComponent:  provisional AABB collider
  *     - CameraTPS:     orbit-mode camera
  *     - PlayerMovement: camera-relative movement + turn smoothing
  *
  *   The cylinder mesh is a placeholder. When Phase 6 (Asset Manager) arrives,
  *   replace with a proper capsule model loaded from file.
  *
  *   The box collider is sized manually to approximate the capsule shape.
  *   Phase 5 replaces this with CapsuleComponent.
  *------------------------------------------------------------------------------------*/
static void SpawnPlayer(Game* game, Vector3 position)
{
    s_player = ACTOR_Create(game);
    s_player->type = ACTOR_TYPE_TPS;
    ACTOR_SetPosition(s_player, position);

    /* Visual: cylinder as capsule placeholder (radius 0.4, height 1.8) */
    MESH_COMPONENT_Create(s_player, &s_meshCapsule, &s_matPlayer);

    /* Collision: box approximating the capsule shape */
    BoxComponent* bc = BOX_COMPONENT_Create(s_player);
    BoundingBox playerBox = {
        .min = { -0.4f, 0.0f, -0.4f },
        .max = {  0.4f, 1.8f,  0.4f }
    };
    BOX_COMPONENT_SetObjectBox(bc, playerBox);

    /* Camera: orbit mode, starts behind player */
    s_camera = CAMERA_TPS_Create(s_player);
    CAMERA_TPS_SetDistances(s_camera,
        CAMERA_HORZ_PRESETS[s_cameraPreset],
        CAMERA_VERT_PRESETS[s_cameraPreset],
        2.0f);
    CAMERA_TPS_SetSpring(s_camera, 128.0f);

    /* Movement: camera-relative, Uncharted/BotW style */
    s_movement = PLAYER_MOVEMENT_Create(s_player, s_camera);
    PLAYER_MOVEMENT_SetMoveSpeed(s_movement, 8.0f);
    PLAYER_MOVEMENT_SetTurnSpeed(s_movement, 15.0f);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Zone Builders
 * ═══════════════════════════════════════════════════════════════════════════ */

 /*------------------------------------------------------------------------------------
  * BuildMovementZone
  *
  *   Large open floor for testing movement and camera. No obstacles — just
  *   a flat plane to verify forward/strafe/diagonal movement directions
  *   and turn smoothing feel correct.
  *------------------------------------------------------------------------------------*/
static void BuildMovementZone(Game* game)
{
    /* Ground plane (40x40) */
    SpawnStaticActor(game, &s_meshPlane, &s_matFloor,
        ZONE_MOVEMENT_CENTER, (Vector3) { 0 }, 1.0f, false);
}

/*------------------------------------------------------------------------------------
 * BuildCollisionZone
 *
 *   Walls, columns, and a ramp to test:
 *     - BoxComponent collision (walk into walls — no response yet, Phase 5)
 *     - CameraTPS wall clamping (camera avoids going behind walls)
 *     - Ramp for future slope/ground detection tests
 *
 *   Layout (top-down, each unit ~2m):
 *       [Wall North]
 *       [Column]  [open]  [Column]
 *       [open]    [Ramp]  [open]
 *       [Wall South corridor]
 *------------------------------------------------------------------------------------*/
static void BuildCollisionZone(Game* game)
{
    Vector3 base = ZONE_COLLISION_CENTER;

    /* Ground plane */
    SpawnStaticActor(game, &s_meshPlane, &s_matFloor,
        base, (Vector3) { 0 }, 1.0f, false);

    /* North wall */
    SpawnStaticActor(game, &s_meshCube, &s_matWall,
        (Vector3) {
        base.x, 1.5f, base.z + 15.0f
    },
        (Vector3) {
        0
    }, 1.0f, true);
    ACTOR_SetScale(
        game->actors[game->actorCount - 1], 1.0f);
    /* Scale the wall — we set position and use a wider box manually */
    Actor* northWall = game->actors[game->actorCount - 1];
    /* Re-scale via root to make it a long wall */
    northWall->root.scale = (Vector3){ 15.0f, 3.0f, 0.5f };
    SCENE_COMPONENT_MarkDirty(&northWall->root);

    /* Two columns */
    SpawnStaticActor(game, &s_meshCube, &s_matWall,
        (Vector3) {
        base.x - 5.0f, 1.5f, base.z + 5.0f
    },
        (Vector3) {
        0
    }, 1.0f, true);
    Actor* col1 = game->actors[game->actorCount - 1];
    col1->root.scale = (Vector3){ 1.0f, 3.0f, 1.0f };
    SCENE_COMPONENT_MarkDirty(&col1->root);

    SpawnStaticActor(game, &s_meshCube, &s_matWall,
        (Vector3) {
        base.x + 5.0f, 1.5f, base.z + 5.0f
    },
        (Vector3) {
        0
    }, 1.0f, true);
    Actor* col2 = game->actors[game->actorCount - 1];
    col2->root.scale = (Vector3){ 1.0f, 3.0f, 1.0f };
    SCENE_COMPONENT_MarkDirty(&col2->root);

    /* Corridor walls (narrow passage for camera stress test) */
    SpawnStaticActor(game, &s_meshCube, &s_matWall,
        (Vector3) {
        base.x - 2.0f, 1.5f, base.z - 5.0f
    },
        (Vector3) {
        0
    }, 1.0f, true);
    Actor* corrL = game->actors[game->actorCount - 1];
    corrL->root.scale = (Vector3){ 0.5f, 3.0f, 8.0f };
    SCENE_COMPONENT_MarkDirty(&corrL->root);

    SpawnStaticActor(game, &s_meshCube, &s_matWall,
        (Vector3) {
        base.x + 2.0f, 1.5f, base.z - 5.0f
    },
        (Vector3) {
        0
    }, 1.0f, true);
    Actor* corrR = game->actors[game->actorCount - 1];
    corrR->root.scale = (Vector3){ 0.5f, 3.0f, 8.0f };
    SCENE_COMPONENT_MarkDirty(&corrR->root);

    /* Ramp (rotated cube) */
    SpawnStaticActor(game, &s_meshCube, &s_matRamp,
        (Vector3) {
        base.x, 0.75f, base.z
    },
        (Vector3) {
        -20.0f * DEG2RAD, 0.0f, 0.0f
    },
        1.0f, true);
    Actor* ramp = game->actors[game->actorCount - 1];
    ramp->root.scale = (Vector3){ 4.0f, 0.3f, 6.0f };
    SCENE_COMPONENT_MarkDirty(&ramp->root);
}

/*------------------------------------------------------------------------------------
 * BuildSpawnZone
 *
 *   Empty floor with boundary markers. The player uses hotkeys to spawn
 *   cubes and spheres here. This zone tests dynamic actor creation,
 *   MeshComponent + BoxComponent/SphereComponent registration, and cleanup.
 *------------------------------------------------------------------------------------*/
static void BuildSpawnZone(Game* game)
{
    Vector3 base = ZONE_SPAWN_CENTER;

    /* Ground plane */
    SpawnStaticActor(game, &s_meshPlane, &s_matFloor,
        base, (Vector3) { 0 }, 1.0f, false);

    /* Corner markers (small cubes) */
    float halfSize = 15.0f;
    Color markerColor = ORANGE;
    Material matMarker = CreateColorMaterial(markerColor);
    /* We reuse s_matSpawnCube for markers since they're temporary visual aids */
    for (int x = -1; x <= 1; x += 2)
    {
        for (int z = -1; z <= 1; z += 2)
        {
            SpawnStaticActor(game, &s_meshCube, &s_matSpawnCube,
                (Vector3) {
                base.x + halfSize * x, 0.5f, base.z + halfSize * z
            },
                (Vector3) {
                0
            }, 0.5f, false);
        }
    }
}

/*------------------------------------------------------------------------------------
 * BuildStressZone
 *
 *   Empty floor for mass-spawning. Tests performance with many actors:
 *   rendering (draw calls, frustum culling), physics (broadphase with many
 *   colliders), and memory (pool exhaustion).
 *------------------------------------------------------------------------------------*/
static void BuildStressZone(Game* game)
{
    Vector3 base = ZONE_STRESS_CENTER;

    /* Ground plane */
    SpawnStaticActor(game, &s_meshPlane, &s_matFloor,
        base, (Vector3) { 0 }, 1.0f, false);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Spawn System
 * ═══════════════════════════════════════════════════════════════════════════ */

static void TrackSpawned(Actor* actor)
{
    if (s_spawnedCount < MAX_SPAWNED)
    {
        s_spawnedActors[s_spawnedCount++] = actor;
    }
}

static void SpawnCubeInFront(Game* game)
{
    if (!s_player) return;

    Vector3 fwd = ACTOR_GetForward(s_player);
    Vector3 pos = Vector3Add(s_player->root.position,
        Vector3Scale(fwd, SPAWN_DISTANCE));
    pos.y = 0.5f;  /* Half-height of a unit cube */

    Actor* cube = SpawnStaticActor(game, &s_meshCube, &s_matSpawnCube,
        pos, (Vector3) { 0 }, 1.0f, true);
    TrackSpawned(cube);
}

static void SpawnSphereInFront(Game* game)
{
    if (!s_player) return;

    Vector3 fwd = ACTOR_GetForward(s_player);
    Vector3 pos = Vector3Add(s_player->root.position,
        Vector3Scale(fwd, SPAWN_DISTANCE));
    pos.y = 0.5f;

    Actor* sphere = ACTOR_Create(game);
    ACTOR_SetPosition(sphere, pos);

    MESH_COMPONENT_Create(sphere, &s_meshSphere, &s_matSpawnSphere);

    SphereComponent* sc = SPHERE_COMPONENT_Create(sphere);
    SPHERE_COMPONENT_Set(sc, (Vector3) { 0, 0.5f, 0 }, 0.5f);

    TrackSpawned(sphere);
}

static void StressSpawn(Game* game)
{
    Vector3 base = ZONE_STRESS_CENTER;

    for (int i = 0; i < STRESS_SPAWN_COUNT; i++)
    {
        float x = base.x + (i % 5) * STRESS_SPAWN_SPACING - 5.0f;
        float z = base.z + (i / 5) * STRESS_SPAWN_SPACING - 5.0f;

        Actor* cube = SpawnStaticActor(game, &s_meshCube, &s_matSpawnCube,
            (Vector3) {
            x, 0.5f, z
        }, (Vector3) { 0 }, 1.0f, true);
        TrackSpawned(cube);
    }
}

static void ClearSpawned(void)
{
    for (int i = 0; i < s_spawnedCount; i++)
    {
        if (s_spawnedActors[i] && s_spawnedActors[i]->state != ACTOR_STATE_DEAD)
        {
            s_spawnedActors[i]->state = ACTOR_STATE_DEAD;
        }
        s_spawnedActors[i] = NULL;
    }
    s_spawnedCount = 0;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Level Callbacks
 * ═══════════════════════════════════════════════════════════════════════════ */

static void Sandbox_Init(Game* game)
{
    /* ── Generate meshes ─────────────────────────────────────────── */
    s_meshCapsule = GenMeshCylinder(0.4f, 1.8f, 12);
    s_meshCube = GenMeshCube(1.0f, 1.0f, 1.0f);
    s_meshSphere = GenMeshSphere(0.5f, 16, 16);
    s_meshPlane = GenMeshPlane(40.0f, 40.0f, 4, 4);
    s_meshRamp = GenMeshCube(1.0f, 1.0f, 1.0f);

    /* ── Create materials ────────────────────────────────────────── */
    s_matPlayer = CreateColorMaterial(SKYBLUE);
    s_matFloor = CreateColorMaterial((Color) { 45, 45, 55, 255 });
    s_matWall = CreateColorMaterial((Color) { 80, 80, 90, 255 });
    s_matSpawnCube = CreateColorMaterial(GREEN);
    s_matSpawnSphere = CreateColorMaterial(YELLOW);
    s_matRamp = CreateColorMaterial((Color) { 120, 80, 60, 255 });

    /* ── Build zones ─────────────────────────────────────────────── */
    BuildMovementZone(game);
    BuildCollisionZone(game);
    BuildSpawnZone(game);
    BuildStressZone(game);

    /* ── Spawn player in movement zone ───────────────────────────── */
    SpawnPlayer(game, (Vector3) { 0.0f, 0.0f, 5.0f });

    /* ── Lock cursor for mouse-look ──────────────────────────────── */
    DisableCursor();

    /* ── Set background color ────────────────────────────────────── */
    RENDERER_SetClearColor(&game->renderer, (Color) { 15, 15, 25, 255 });

    TraceLog(LOG_INFO, "SANDBOX: Initialized — WASD move, Mouse look, 1/2/3/X spawn, Tab camera, F2-F4 debug");
}

static void Sandbox_Shutdown(Game* game)
{
    (void)game;

    /* Clear spawn tracking */
    s_spawnedCount = 0;

    /* Null out references (actors destroyed by GAME_RemoveAllActors) */
    s_player = NULL;
    s_camera = NULL;
    s_movement = NULL;

    /* Restore cursor */
    EnableCursor();

    /* Free meshes */
    UnloadMesh(s_meshCapsule);
    UnloadMesh(s_meshCube);
    UnloadMesh(s_meshSphere);
    UnloadMesh(s_meshPlane);
    UnloadMesh(s_meshRamp);

    /* Free materials */
    UnloadMaterial(s_matPlayer);
    UnloadMaterial(s_matFloor);
    UnloadMaterial(s_matWall);
    UnloadMaterial(s_matSpawnCube);
    UnloadMaterial(s_matSpawnSphere);
    UnloadMaterial(s_matRamp);

    TraceLog(LOG_INFO, "SANDBOX: Shutdown complete");
}

/*------------------------------------------------------------------------------------
 * Sandbox_ProcessInput
 *
 *   Runs once per frame (not per fixed-step). Handles:
 *     - Mouse delta → camera orbit rotation
 *     - Hotkeys for spawning, cleanup, camera distance, debug toggles
 *
 *   Movement input (WASD) is NOT here — it's read inside PlayerMovement's
 *   Update at fixed-step rate, which is correct for continuous movement.
 *   Camera rotation IS here because mouse deltas are per-frame events.
 *------------------------------------------------------------------------------------*/
static void Sandbox_ProcessInput(Game* game)
{
    /* ── Camera orbit from mouse ─────────────────────────────────── */
    if (s_camera)
    {
        Vector2 mouseDelta = GetMouseDelta();
        CAMERA_TPS_RotateOrbit(s_camera, mouseDelta.x, mouseDelta.y);
    }

    /* ── Spawn hotkeys ───────────────────────────────────────────── */
    if (IsKeyPressed(KEY_ONE))   SpawnCubeInFront(game);
    if (IsKeyPressed(KEY_TWO))   SpawnSphereInFront(game);
    if (IsKeyPressed(KEY_THREE)) StressSpawn(game);
    if (IsKeyPressed(KEY_X))     ClearSpawned();

    /* ── Camera distance cycle ───────────────────────────────────── */
    if (IsKeyPressed(KEY_TAB) && s_camera)
    {
        s_cameraPreset = (s_cameraPreset + 1) % CAMERA_PRESET_COUNT;
        CAMERA_TPS_SetDistances(s_camera,
            CAMERA_HORZ_PRESETS[s_cameraPreset],
            CAMERA_VERT_PRESETS[s_cameraPreset],
            2.0f);
    }

    /* ── Debug toggles ───────────────────────────────────────────── */
    if (IsKeyPressed(KEY_F2)) s_showGrid = !s_showGrid;
    if (IsKeyPressed(KEY_F3)) s_showColliders = !s_showColliders;
    if (IsKeyPressed(KEY_F4)) s_showZoneLabels = !s_showZoneLabels;
}

/*------------------------------------------------------------------------------------
 * Sandbox_Render3D
 *
 *   Called inside BeginMode3D. Draws debug visuals that are part of the
 *   3D world (not HUD text — that goes in RenderHUD).
 *------------------------------------------------------------------------------------*/
static void Sandbox_Render3D(Game* game)
{
    (void)game;

    /* ── Ground grid ─────────────────────────────────────────────── */
    if (s_showGrid)
    {
        DrawGrid(80, 2.0f);
    }

    /* ── Collider wireframes ─────────────────────────────────────── */
    if (s_showColliders)
    {
        /* Draw all box colliders */
        for (int i = 0; i < game->physWorld.boxCount; i++)
        {
            BOX_COMPONENT_DrawWorldBox(game->physWorld.boxes[i], RED);
        }
        /* Draw all sphere colliders */
        for (int i = 0; i < game->physWorld.sphereCount; i++)
        {
            SPHERE_COMPONENT_DrawWires(game->physWorld.spheres[i], BLUE);
        }
    }

    /* ── Zone boundary markers (simple lines on the ground) ──────── */
    if (s_showZoneLabels)
    {
        /* Draw zone boundary lines at XZ plane */
        /* Vertical divider (between movement and collision zones) */
        DrawLine3D(
            (Vector3) {
            15.0f, 0.05f, 20.0f
        },
            (Vector3) {
            15.0f, 0.05f, -50.0f
        },
            (Color) {
            255, 255, 255, 60
        });
        /* Horizontal divider (between movement and spawn zones) */
        DrawLine3D(
            (Vector3) {
            -20.0f, 0.05f, -15.0f
        },
            (Vector3) {
            50.0f, 0.05f, -15.0f
        },
            (Color) {
            255, 255, 255, 60
        });
    }
}

/*------------------------------------------------------------------------------------
 * Sandbox_RenderHUD
 *
 *   2D overlay with controls help, zone labels, and spawn count.
 *------------------------------------------------------------------------------------*/
static int Sandbox_RenderHUD(Game* game, int y)
{
    (void)game;
    int x = 10;
    int lineH = 18;
    Color textCol = RAYWHITE;
    Color dimCol = (Color){ 150, 150, 150, 200 };

    /* ── Title ───────────────────────────────────────────────────── */
    DrawText("SANDBOX", x, y, 20, textCol);
    y += 28;

    /* ── Player info ─────────────────────────────────────────────── */
    if (s_player)
    {
        Vector3 pos = s_player->root.position;
        char buf[128];
        snprintf(buf, sizeof(buf), "Pos: %.1f, %.1f, %.1f  %s",
            pos.x, pos.y, pos.z,
            PLAYER_MOVEMENT_IsMoving(s_movement) ? "[MOVING]" : "[IDLE]");
        DrawText(buf, x, y, 10, textCol);
        y += lineH;
    }

    /* ── Spawn count ─────────────────────────────────────────────── */
    char spawnBuf[64];
    snprintf(spawnBuf, sizeof(spawnBuf), "Spawned: %d / %d", s_spawnedCount, MAX_SPAWNED);
    DrawText(spawnBuf, x, y, 10, textCol);
    y += lineH;

    /* ── Camera preset ───────────────────────────────────────────── */
    const char* presetNames[] = { "Close", "Medium", "Far" };
    char camBuf[64];
    snprintf(camBuf, sizeof(camBuf), "Camera: %s (Tab to cycle)", presetNames[s_cameraPreset]);
    DrawText(camBuf, x, y, 10, textCol);
    y += lineH + 4;

    /* ── Debug toggle status ─────────────────────────────────────── */
    char dbgBuf[128];
    snprintf(dbgBuf, sizeof(dbgBuf), "F2 Grid:%s  F3 Colliders:%s  F4 Zones:%s",
        s_showGrid ? "ON" : "off",
        s_showColliders ? "ON" : "off",
        s_showZoneLabels ? "ON" : "off");
    DrawText(dbgBuf, x, y, 10, dimCol);
    y += lineH;

    /* ── Controls ────────────────────────────────────────────────── */
    DrawText("WASD move | Mouse look | 1 cube | 2 sphere | 3 stress | X clear",
        x, y, 10, dimCol);
    y += lineH;

    return y;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Level Definition
 * ═══════════════════════════════════════════════════════════════════════════ */

Level LEVEL_SANDBOX = {
    .name = "Sandbox",
    .Init = Sandbox_Init,
    .Shutdown = Sandbox_Shutdown,
    .ProcessInput = Sandbox_ProcessInput,
    .Render3D = Sandbox_Render3D,
    .RenderHUD = Sandbox_RenderHUD,
};