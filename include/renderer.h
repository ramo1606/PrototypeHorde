#pragma once

#include "raylib.h"
#include "raymath.h"
#include "config.h"
#include <stdbool.h>

typedef struct Game Game;
typedef struct Renderer Renderer;
typedef struct Renderable Renderable;
typedef struct FrustumPlane FrustumPlane;
typedef struct DrawEntry DrawEntry;

/* ── Configuration ───────────────────────────────────────────────────────── */

#define RENDERER_NEAR_PLANE     0.01f
#define RENDERER_FAR_PLANE      1000.0f

/* ── Handle ──────────────────────────────────────────────────────────────── */

/* A RenderHandle is an index into the renderables array.
 * RENDER_HANDLE_INVALID means "not registered" or "registration failed". */
typedef int RenderHandle;
#define RENDER_HANDLE_INVALID (-1)

/* ── Renderable ──────────────────────────────────────────────────────────── */

/*
 * Everything the renderer needs to draw one object.
 * Gameplay code fills transformCurr each tick; the renderer copies
 * curr→prev automatically at the start of each tick (Task 1.2).
 *
 * The model is stored by value because raylib's Model is already
 * a lightweight struct (pointers to mesh/material data on GPU).
 * Copying it is cheap — it doesn't duplicate the GPU resources.
 */
struct Renderable 
{
    Model model;                /* Raylib model to draw                     */
    Matrix transformCurr;       /* Current tick's world transform           */
    Matrix transformPrev;       /* Previous tick's transform (interpolation)*/
    Vector3 boundingCenter;     /* Local-space center for culling           */
    float boundingRadius;       /* Bounding sphere radius for culling       */
    int materialID;             /* For sorting (group by shader/texture)    */
    bool active;                /* Slot in use?                             */
};

/* ── Frustum Plane ───────────────────────────────────────────────────────── */

/* Plane in Hessian normal form: ax + by + cz + d = 0 */
struct FrustumPlane
{
    float a, b, c, d;
};

/* ── Draw Entry ──────────────────────────────────────────────────────────── */

/* Entry in the sorted draw list: renderable index + sort key. */
struct DrawEntry 
{
    int index;                  /* Index into renderables[]                 */
    int materialID;             /* Copied from renderable (sort key)        */
    float distSq;               /* Squared distance to camera (for sorting) */
    Matrix transform;           /* Interpolated world transform (cached)    */
};

/* ── Renderer Struct ─────────────────────────────────────────────────────── */

struct Renderer 
{
    /* Camera */
    Camera3D camera;
    Color clearColor;

    Shader celShader;               /* Loaded via resource manager          */
    int locLightDir;                /* Uniform: vec3 light direction        */
    int locAmbient;                 /* Uniform: float ambient level         */
    int locNumBands;                /* Uniform: float band count            */
    Vector3 lightDir;               /* Current light direction (normalized) */
    float ambient;                  /* Current ambient level [0..1]         */
    float numBands;                 /* Current band count (e.g. 3.0)        */

    /* Renderable pool */
    Renderable renderables[MAX_RENDERABLES];
    int renderableCount;                /* Number of active renderables         */

    /* Draw list (built each frame after culling) */
    DrawEntry drawList[MAX_RENDERABLES];
    int drawCount;                      /* Entries in draw list this frame      */

    /* Frustum planes (extracted each frame) */
    FrustumPlane frustum[6];        /* Left, Right, Bottom, Top, Near, Far  */

    /* Per-frame stats (for debug overlay) */
    int statsDrawn;
    int statsCulled;
};

/* ── Lifecycle ───────────────────────────────────────────────────────────── */

void RENDERER_Init(Renderer* renderer);
void RENDERER_Shutdown(Renderer* renderer);

/* ── Registration ────────────────────────────────────────────────────────── */

RenderHandle RENDERER_Register(Renderer* renderer, Model model, int materialID);
void RENDERER_Unregister(Renderer* renderer, RenderHandle handle);

/* ── Transform Updates (called by gameplay code each tick) ───────────────── */

void RENDERER_SetTransform(Renderer* renderer, RenderHandle handle, Matrix transform);
void RENDERER_PreUpdate(Renderer* renderer);

/* ── Frame Drawing (split pipeline) ──────────────────────────────────────── */

void RENDERER_BuildDrawList(Renderer* renderer, float alpha);
void RENDERER_Draw3D(Renderer* renderer);

/* ── Camera Control ──────────────────────────────────────────────────────── */

void RENDERER_SetCamera(Renderer* renderer, Camera3D camera);
Camera3D RENDERER_GetCamera(const Renderer* renderer);
void RENDERER_SetClearColor(Renderer* renderer, Color color);

/* ── Lighting (Cel Shader) ───────────────────────────────────────────────── */

void RENDERER_SetLightDir(Renderer* renderer, Vector3 dir);
void RENDERER_SetAmbient(Renderer* renderer, float ambient);
void RENDERER_SetNumBands(Renderer* renderer, float numBands);

/* ── Frustum Culling ────────── */

void RENDERER_ExtractFrustumPlanes(FrustumPlane planes[6], Matrix m);
bool RENDERER_IsSphereInFrustum(const FrustumPlane planes[6], Vector3 center, float radius);

/* ── Bounding Sphere Helpers ─────────────────────────────────────────────── */

void RENDERER_ComputeBoundingSphere(Model model, Vector3* outCenter, float* outRadius);

/* ── Screen Projection (utility) ─────────────────────────────────────────── */

Vector2 RENDERER_WorldToScreen(const Renderer* renderer, Vector3 worldPos);