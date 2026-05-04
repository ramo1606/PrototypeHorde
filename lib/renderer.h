#pragma once

/*
 * renderer.h — 3D renderable pool with interpolation, frustum culling,
 *              material sorting, and draw list submission.
 *
 * DEPENDENCIES: raylib.h
 * OVERRIDES (define before include):
 *   RENDERER_MAX_RENDERABLES (default 256)
 *   RENDERER_NEAR_PLANE      (default 0.01f)
 *   RENDERER_FAR_PLANE       (default 1000.0f)
 *
 * The renderer does not own a camera, a clear color, a shader, or any
 * effect. Callers pass the Camera3D to BuildDrawList; shader assignment
 * to models is the caller's job; effects (cel shading, blob shadows,
 * outline) draw after RendererDraw3D using the public draw list.
 */

#include "raylib.h"
#include <stdbool.h>

#ifndef RENDERER_MAX_RENDERABLES
#  define RENDERER_MAX_RENDERABLES 256
#endif

#ifndef RENDERER_NEAR_PLANE
#  define RENDERER_NEAR_PLANE 0.01f
#endif

#ifndef RENDERER_FAR_PLANE
#  define RENDERER_FAR_PLANE 1000.0f
#endif

/* A RenderHandle is an index into the renderables array.
 * RENDER_HANDLE_INVALID means "not registered" or "registration failed". */
typedef int RenderHandle;
#define RENDER_HANDLE_INVALID (-1)

typedef struct Renderable
{
    Model   model;
    Matrix  transformCurr;       /* Current tick's world transform           */
    Matrix  transformPrev;       /* Previous tick's transform (interpolation)*/
    Vector3 boundingCenter;      /* Local-space center for culling           */
    float   boundingRadius;
    int     materialID;          /* For sorting (group by shader/texture)    */
    bool    active;
} Renderable;

/* Plane in Hessian normal form: ax + by + cz + d = 0 */
typedef struct FrustumPlane
{
    float a, b, c, d;
} FrustumPlane;

/* Entry in the sorted draw list: renderable index + sort key.
 * Public so effects passes (blob shadows, outlines) can iterate visible
 * objects with their interpolated transforms. */
typedef struct DrawEntry
{
    int    index;                /* Index into renderables[] */
    int    materialID;
    float  distSq;               /* Squared distance to camera */
    Matrix transform;            /* Cached interpolated transform */
} DrawEntry;

typedef struct Renderer
{
    /* Renderable pool */
    Renderable renderables[RENDERER_MAX_RENDERABLES];
    int        renderableCount;

    /* Draw list (built each frame after culling) */
    DrawEntry drawList[RENDERER_MAX_RENDERABLES];
    int       drawCount;

    /* Frustum planes (extracted each frame): 0=L 1=R 2=B 3=T 4=N 5=F */
    FrustumPlane frustum[6];

    /* Per-frame stats */
    int statsDrawn;
    int statsCulled;
} Renderer;

/* ── Lifecycle ───────────────────────────────────────────────────────────── */

void RendererInit(Renderer* renderer);
void RendererShutdown(Renderer* renderer);

/* ── Registration ────────────────────────────────────────────────────────── */

/* Register a model. The renderer does NOT assign a shader; do that on
 * the model before registering if you want a custom one. */
RenderHandle RendererRegister(Renderer* renderer, Model model, int materialID);
void         RendererUnregister(Renderer* renderer, RenderHandle handle);

/* ── Transform Updates ───────────────────────────────────────────────────── */

void RendererSetTransform(Renderer* renderer, RenderHandle handle, Matrix transform);

/* Copy curr → prev for all active renderables. Call once at the start
 * of every fixed tick, BEFORE gameplay updates. */
void RendererPreUpdate(Renderer* renderer);

/* ── Frame Drawing ───────────────────────────────────────────────────────── */

/* Interpolate, frustum-cull, sort. Once per visual frame, BEFORE BeginMode3D.
 * The camera is passed in so the renderer doesn't need to own it. */
void RendererBuildDrawList(Renderer* renderer, Camera3D camera, float alpha);

/* Submit draw calls for the sorted list. Call inside BeginMode3D. */
void RendererDraw3D(Renderer* renderer);

/* ── Frustum Culling Helpers (public; useful for effects) ────────────────── */

void RendererExtractFrustumPlanes(FrustumPlane planes[6], Matrix m);
bool RendererIsSphereInFrustum(const FrustumPlane planes[6],
                               Vector3 center, float radius);

/* ── Bounding Sphere Helper (used internally on Register, public utility) ── */

void RendererComputeBoundingSphere(Model model, Vector3* outCenter, float* outRadius);
