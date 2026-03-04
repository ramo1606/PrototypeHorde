/*******************************************************************************************
*
*   renderer.h — Renderer (3D Drawing Pipeline)
*
*   The Renderer manages the 3D drawing pipeline: camera, frustum culling, draw list
*   construction, material sorting, and frame presentation. It maintains a list of
*   all registered MeshComponents and draws them each frame.
*
*   Render Pipeline (per frame):
*       1. Extract frustum planes from view-projection matrix
*       2. Build draw list: cull invisible meshes against frustum
*       3. Sort draw list by material (minimize state changes)
*       4. Begin 3D mode → draw all meshes → draw level overlays → End 3D
*       5. Draw HUD (2D overlay)
*       6. Draw debug overlay
*       7. Draw level transition effects
*       8. End frame
*
*   Frustum Culling:
*       Uses 6 planes extracted from the VP matrix (Gribb-Hartmann method).
*       Each mesh's world AABB is tested against all 6 planes — if fully
*       behind any plane, the mesh is culled (not drawn).
*
*   Architecture:
*       Game
*           └── Renderer (embedded)
*                   ├── camera      → active Camera3D for view/projection
*                   ├── meshes[]    → all registered MeshComponents
*                   ├── drawList[]  → visible meshes after culling
*                   └── frustum[6]  → cached frustum planes
*
*   Naming Convention:
*       API:     RENDERER_*
*
********************************************************************************************/
#pragma once

#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>

typedef struct Game Game;
typedef struct MeshComponent MeshComponent;
typedef struct Renderer Renderer;
typedef struct FrustumPlane FrustumPlane;
typedef struct DrawEntry DrawEntry;

/* ── Configuration ───────────────────────────────────────────────────────── */

#define RENDERER_MAX_MESHES 1024        /* Max registered mesh components     */
#define RENDERER_NEAR_PLANE 0.01        /* Near clipping plane distance       */
#define RENDERER_FAR_PLANE  1000.0      /* Far clipping plane distance        */

/* ── Frustum Plane ───────────────────────────────────────────────────────── */

/* A plane in Hessian normal form: ax + by + cz + d = 0 */
struct FrustumPlane
{
    float a, b, c, d;           /* Normal (a,b,c) and distance from origin (d)   */
};

/* ── Draw Entry ──────────────────────────────────────────────────────────── */

/* Entry in the sorted draw list: mesh + distance to camera for sorting. */
struct DrawEntry
{
    MeshComponent* mc;          /* The mesh component to draw                     */
    float distSq;               /* Squared distance to camera (for potential sort) */
};

/* ── Renderer Struct ─────────────────────────────────────────────────────── */
struct Renderer
{
    Camera3D camera;                                /* Active camera (set by CameraComponent.Apply)    */
    Color clearColor;                               /* Background clear color                          */

    MeshComponent* meshes[RENDERER_MAX_MESHES];     /* All registered meshes          */
    int meshCount;                                  /* Number of registered meshes                     */

    DrawEntry drawList[RENDERER_MAX_MESHES];        /* Visible meshes after culling    */
    int drawCount;                                  /* Number of meshes in draw list                   */

    FrustumPlane frustum[6];                        /* Cached frustum planes (L, R, B, T, N, F)        */

    int statsCulled;                                /* Meshes rejected by frustum this frame            */
    int statsDrawn;                                 /* Meshes drawn this frame                          */
};

/* ── Lifecycle ───────────────────────────────────────────────────────────── */
void RENDERER_Init(Renderer* renderer);
void RENDERER_Shutdown(Renderer* renderer);
void RENDERER_DrawFrame(Renderer* renderer, Game* game);

/* ── Mesh Management ─────────────────────────────────────────────────────── */
void RENDERER_AddMesh(Renderer* renderer, MeshComponent* mc);
void RENDERER_RemoveMesh(Renderer* renderer, MeshComponent* mc);

/* ── Camera Control ──────────────────────────────────────────────────────── */
void RENDERER_SetCamera(Renderer* renderer, Camera3D camera);
Camera3D RENDERER_GetCamera(const Renderer* renderer);
void RENDERER_SetClearColor(Renderer* renderer, Color color);

/* ── Frustum Culling ─────────────────────────────────────────────────────── */
void RENDERER_ExtractFrustumPlanes(FrustumPlane planes[6], Matrix viewProj);
bool RENDERER_IsAABBInFrustum(const FrustumPlane planes[6], BoundingBox box);
bool RENDERER_IsPointInFrustum(const FrustumPlane planes[6], Vector3 point);
bool RENDERER_IsSphereInFrustum(const FrustumPlane planes[6], Vector3 center, float radius);

/* ── Screen Projection ───────────────────────────────────────────────────── */
Vector2 RENDERER_WorldToScreen(const Renderer* renderer, Vector3 worldPos);
bool RENDERER_IsOnScreen(const Renderer* renderer, Vector3 worldPos);