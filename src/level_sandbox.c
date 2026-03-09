/*******************************************************************************************
*
*   level_sandbox.c — Development Sandbox Level
*
*   Step 2: Floor + static camera. Validates that we can create an actor with a
*   MeshComponent, see it on screen, and that the resource lifecycle (create in
*   Init, free in Shutdown) works without leaks.
*
********************************************************************************************/
#include "level.h"
#include "game.h"
#include "actor.h"
#include "camera_tps.h"
#include "player_movement.h"
#include "mesh_component.h"
#include "renderer.h"
#include "raylib.h"

/* ═══════════════════════════════════════════════════════════════════════════
 *  Level-Local State
 *
 *  Meshes and materials are owned by this level because there is no Asset
 *  Manager yet (Phase 6). They are created in Init and freed in Shutdown.
 *  The pointers we pass to MeshComponent point into these statics — they
 *  remain valid for the entire lifetime of the level.
 * ═══════════════════════════════════════════════════════════════════════════ */

static Mesh     s_meshFloor;
static Material s_matFloor;
static Model s_modelPlayer;
static Actor* s_player = NULL;
static CameraTPS* s_camera = NULL;
static PlayerMovement* s_movement = NULL;

/* ═══════════════════════════════════════════════════════════════════════════
 *  Level Callbacks
 * ═══════════════════════════════════════════════════════════════════════════ */

static void Sandbox_Init(Game* game)
{
    /* ── Create resources ────────────────────────────────────────── */
    /*
     * GenMeshPlane(width, length, resX, resZ)
     *   Generates a horizontal plane centered at the origin, lying on Y=0.
     *   40x40 units gives us plenty of room. resX/resZ are subdivisions —
     *   4x4 is enough for a flat floor (more subdivisions are for terrain).
     */
    s_meshFloor = GenMeshPlane(40.0f, 40.0f, 4, 4);

    /*
     * LoadMaterialDefault() gives us Raylib's built-in shader with a
     * white diffuse texture. We override the diffuse color to dark gray
     * so the floor is visible but doesn't compete with other objects.
     */
    s_matFloor = LoadMaterialDefault();
    s_matFloor.maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 45, 45, 55, 255 };

    /* ── Spawn floor actor ───────────────────────────────────────── */
    /*
     * The floor is an Actor with a single MeshComponent. No collider —
     * the floor is visual only for now. Collision with the ground comes
     * in Phase 5 (ground detection via shape casts).
     *
     * Position defaults to (0,0,0) which is where GenMeshPlane centers
     * the geometry, so we don't need to move it.
     */
    Actor* floor = ACTOR_Create(game);
    MeshComponent* floorMesh = MESH_COMPONENT_Create(floor, &s_meshFloor, &s_matFloor);
	MESH_COMPONENT_SetTint(floorMesh, (Color) { 45, 45, 55, 255 });

    /* ── Load player model ───────────────────────────────────────── */
    s_modelPlayer = LoadModel("assets/player.gltf");

    /* ── Spawn player actor (static, no movement yet) ───────────── */
    s_player = ACTOR_Create(game);
    ACTOR_SetPosition(s_player, (Vector3) { 0.0f, 0.0f, 0.0f });
    MeshComponent* mc = MESH_COMPONENT_Create(s_player, &s_modelPlayer.meshes[0], &s_modelPlayer.materials[1]);
    mc->scene.rotation.y = PI;
    SCENE_COMPONENT_MarkDirty(&mc->scene);

    s_camera = CAMERA_TPS_Create(s_player);
    CAMERA_TPS_SetDistances(s_camera, 8.0f, 4.0f, 2.0f);
    CAMERA_TPS_SetSpring(s_camera, 128.0f);

    s_movement = PLAYER_MOVEMENT_Create(s_player, s_camera);

    RENDERER_SetClearColor(&game->renderer, (Color) { 15, 15, 25, 255 });

	DisableCursor();

    TraceLog(LOG_INFO, "SANDBOX: Initialized — floor + static camera");
}

static void Sandbox_ProcessInput(Game* game)
{
    (void)game;
    if (s_camera)
    {
        Vector2 delta = GetMouseDelta();
        CAMERA_TPS_RotateOrbit(s_camera, delta.x, delta.y);
    }
}

static void Sandbox_Render3D(Game* game)
{
    (void)game;
    //DrawGrid(20, 1.0f);
}

static void Sandbox_Shutdown(Game* game)
{
    (void)game;

    /*
     * Free resources in reverse order of creation. The actors (and their
     * MeshComponents) are already destroyed by GAME_RemoveAllActors before
     * Shutdown is called, so the mesh/material pointers they held are no
     * longer referenced by anyone. Safe to free.
     */
    UnloadMaterial(s_matFloor);
    UnloadMesh(s_meshFloor);
    UnloadModel(s_modelPlayer);

    s_player = NULL;
    s_camera = NULL;
    s_movement = NULL;
    EnableCursor();

    TraceLog(LOG_INFO, "SANDBOX: Shutdown complete");
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
    .RenderHUD = NULL,
};