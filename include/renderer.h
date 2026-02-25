#pragma once

/*
 * renderer.h — 3D scene renderer with frustum culling and draw-list sorting.
 *
 * The Renderer owns the active Camera3D and a flat list of all registered
 * MeshComponents.  Each call to RENDERER_DrawFrame executes the following
 * pipeline:
 *
 *   1. Extract 6 frustum planes from the view-projection matrix using the
 *      Gribb-Hartmann method (combine VP matrix rows).
 *   2. Build draw list: skip invisible/inactive meshes; AABB frustum cull
 *      the rest; record squared distance to camera.
 *   3. Sort draw list by material pointer to minimise GPU state changes.
 *   4. Draw all entries, then hand off to the active level for 3D overlays
 *      and HUD.
 *
 * Gribb-Hartmann frustum extraction reference:
 *   "Fast Extraction of Viewing Frustum Planes from the World-View-
 *    Projection Matrix" — Gribb & Hartmann, 2001.
 *
 * Architecture position:
 *   Game.renderer (value, not pointer)
 *   MeshComponent registers/unregisters itself on create/destroy.
 */

#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>

typedef struct Game Game;
typedef struct MeshComponent MeshComponent;
typedef struct Renderer Renderer;
typedef struct FrustumPlane FrustumPlane;
typedef struct DrawEntry DrawEntry;

/* ── Renderer Constants ─────────────────────────────────────────── */

#define RENDERER_MAX_MESHES 1024   /* Maximum number of MeshComponents that can be registered */

#define RENDERER_NEAR_PLANE 0.01   /* Near clip plane distance (world units) */
#define RENDERER_FAR_PLANE  1000.0 /* Far clip plane distance (world units) */

/* ── Internal Structs ───────────────────────────────────────────── */

struct FrustumPlane
{
    float a, b, c; /* Plane normal components (normalised) */
    float d;       /* Plane distance from origin (signed) */
};

struct DrawEntry
{
    MeshComponent* mc;    /* Pointer to the mesh component to draw */
    float          distSq; /* Squared distance from camera to mesh AABB centre (used for future depth sort) */
};

/* ── Renderer Struct ────────────────────────────────────────────── */

struct Renderer
{
    Camera3D camera;     /* Active camera used for the view and projection transforms */
    Color    clearColor; /* Background clear colour (changes with game state) */

    MeshComponent* meshes[RENDERER_MAX_MESHES]; /* All registered mesh components */
    int            meshCount;                   /* Number of active entries in meshes[] */

    DrawEntry drawList[RENDERER_MAX_MESHES]; /* Frustum-culled subset of meshes[], rebuilt each frame */
    int       drawCount;                     /* Number of entries in the current draw list */

    FrustumPlane frustum[6]; /* Six planes of the view frustum (left, right, bottom, top, near, far) */

    int statsCulled; /* Number of meshes culled by frustum this frame (debug counter) */
    int statsDrawn;  /* Number of meshes actually submitted for drawing this frame (debug counter) */
};

/* ── Lifecycle ──────────────────────────────────────────────────── */

void RENDERER_Init(Renderer* renderer);                   // Initialise the renderer with a default camera and clear colour
void RENDERER_Shutdown(Renderer* renderer);               // Shut down the renderer (currently a no-op but reserved for future GPU resource cleanup)
void RENDERER_DrawFrame(Renderer* renderer, Game* game);  // Execute the full render pipeline: frustum cull → sort → 3D draw → HUD → debug

/* ── Mesh Registration ──────────────────────────────────────────── */

void RENDERER_AddMesh(Renderer* renderer, MeshComponent* mc);    // Register a MeshComponent into the renderable list
void RENDERER_RemoveMesh(Renderer* renderer, MeshComponent* mc); // Unregister a MeshComponent (swap-remove, O(n))

/* ── Camera ─────────────────────────────────────────────────────── */

void     RENDERER_SetCamera(Renderer* renderer, Camera3D camera);        // Replace the active camera (called by CameraComponent/CameraTPS each frame)
Camera3D RENDERER_GetCamera(const Renderer* renderer);                   // Return a copy of the current active camera
void     RENDERER_SetClearColor(Renderer* renderer, Color color);        // Set the background clear colour for the next frame

/* ── Frustum Culling ────────────────────────────────────────────── */

void RENDERER_ExtractFrustumPlanes(FrustumPlane planes[6], Matrix viewProj);                      // Extract and normalise 6 frustum planes from a combined view-projection matrix (Gribb-Hartmann)
bool RENDERER_IsAABBInFrustum(const FrustumPlane planes[6], BoundingBox box);                     // Return true if the AABB is fully or partially inside the frustum (positive-vertex test)
bool RENDERER_IsPointInFrustum(const FrustumPlane planes[6], Vector3 point);                      // Return true if the point is on the positive side of all 6 frustum planes
bool RENDERER_IsSphereInFrustum(const FrustumPlane planes[6], Vector3 center, float radius);      // Return true if the sphere overlaps or is inside the frustum

/* ── Screen-Space Utilities ─────────────────────────────────────── */

Vector2 RENDERER_WorldToScreen(const Renderer* renderer, Vector3 worldPos); // Project a world-space point to screen-space pixels; returns (-1,-1) if behind the camera
bool    RENDERER_IsOnScreen(const Renderer* renderer, Vector3 worldPos);    // Return true if the world-space point projects within the viewport bounds