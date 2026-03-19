#include "renderer.h"
#include "raylib.h"
#include "raymath.h"

#include <assert.h>
#include <string.h>
#include <math.h>

/*******************************************************************************************
*
*   renderer.c — Centralized 3D Rendering Pipeline
*
*   Task 1.1: Basic registration, transform updates, and draw loop.
*   Task 1.2: Interpolation (PreUpdate + lerp in DrawFrame).
*   Task 1.3: Frustum culling (ExtractFrustumPlanes + IsSphereInFrustum).
*   Task 1.4: Material sorting (qsort on draw list).
*
********************************************************************************************/

/* ═══════════════════════════════════════════════════════════════════════════
 *  Lifecycle
 * ═══════════════════════════════════════════════════════════════════════════ */

void RENDERER_Init(Renderer* renderer)
{
    assert(renderer);
    memset(renderer, 0, sizeof(*renderer));

    renderer->clearColor = (Color){ 20, 20, 40, 255 };

    renderer->camera = (Camera3D){
        .position = (Vector3){ 10.0f, 8.0f, 10.0f },
        .target = (Vector3){ 0.0f, 0.0f, 0.0f },
        .up = (Vector3){ 0.0f, 1.0f, 0.0f },
        .fovy = 60.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    TraceLog(LOG_INFO, "RENDERER: Initialized (max %d renderables)", MAX_RENDERABLES);
}

void RENDERER_Shutdown(Renderer* renderer)
{
    assert(renderer);
    memset(renderer->renderables, 0, sizeof(renderer->renderables));
    renderer->renderableCount = 0;
    TraceLog(LOG_INFO, "RENDERER: Shutdown");
}

RenderHandle RENDERER_Register(Renderer* renderer, Model model, int materialID)
{
    assert(renderer);

    /* Find first inactive slot */
    for (int i = 0; i < MAX_RENDERABLES; i++) 
    {
        if (!renderer->renderables[i].active) 
        {
            Renderable* r = &renderer->renderables[i];
            memset(r, 0, sizeof(*r));

            r->model = model;
            r->materialID = materialID;
            r->active = true;

            /* Start with identity transform */
            r->transformCurr = MatrixIdentity();
            r->transformPrev = MatrixIdentity();

            /* Compute bounding sphere for frustum culling */
            RENDERER_ComputeBoundingSphere(model,
                &r->boundingCenter,
                &r->boundingRadius);

            renderer->renderableCount++;

            TraceLog(LOG_DEBUG, "RENDERER: Registered handle %d (total: %d)",
                i, renderer->renderableCount);
            return i;
        }
    }

    TraceLog(LOG_WARNING, "RENDERER: Pool full, cannot register (max %d)",
        MAX_RENDERABLES);
    return RENDER_HANDLE_INVALID;
}

void RENDERER_Unregister(Renderer* renderer, RenderHandle handle)
{
    assert(renderer);

    if (handle == RENDER_HANDLE_INVALID) return;
    if (handle < 0 || handle >= MAX_RENDERABLES) return;
    if (!renderer->renderables[handle].active) return;

    renderer->renderables[handle].active = false;
    renderer->renderableCount--;

    TraceLog(LOG_DEBUG, "RENDERER: Unregistered handle %d (total: %d)",
        handle, renderer->renderableCount);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Transform Updates
 * ═══════════════════════════════════════════════════════════════════════════ */

void RENDERER_SetTransform(Renderer* renderer, RenderHandle handle, Matrix transform)
{
    assert(renderer);
    if (handle < 0 || handle >= MAX_RENDERABLES) return;
    if (!renderer->renderables[handle].active) return;

    renderer->renderables[handle].transformCurr = transform;
}

/*
 * Copy curr→prev for all active renderables.
 * Called ONCE at the start of each logic tick, BEFORE gameplay updates.
 *
 * Why: gameplay code sets transformCurr during its update. The renderer
 * needs transformPrev (from last tick) and transformCurr (this tick) to
 * interpolate between them. If we copied curr→prev AFTER gameplay, prev
 * and curr would be identical and there'd be nothing to interpolate.
 */
void RENDERER_PreUpdate(Renderer* renderer)
{
    assert(renderer);

    for (int i = 0; i < MAX_RENDERABLES; i++)
    {
        if (renderer->renderables[i].active)
        {
            renderer->renderables[i].transformPrev = renderer->renderables[i].transformCurr;
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Interpolation Helper
 *
 *  Linearly interpolate between two transform matrices.
 *  This is a simplification — true matrix interpolation should decompose
 *  into T/R/S and lerp position, slerp rotation. For our purposes (small
 *  steps at 60Hz), lerping the matrix elements directly is good enough
 *  and significantly cheaper.
 * ═══════════════════════════════════════════════════════════════════════════ */

static Matrix LerpMatrix(Matrix a, Matrix b, float t)
{
    Matrix result;
    float* ra = (float*)&a;
    float* rb = (float*)&b;
    float* rr = (float*)&result;

    for (int i = 0; i < 16; i++) 
    {
        rr[i] = ra[i] + (rb[i] - ra[i]) * t;
    }
    return result;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Frame Drawing
 *
 *  Task 1.1: Draw all active renderables (no culling, no sorting).
 *  Task 1.2: Add interpolation with alpha.
 *  Task 1.3: Add frustum culling before drawing.
 *  Task 1.4: Add material sorting of draw list.
 * ═══════════════════════════════════════════════════════════════════════════ */
void RENDERER_DrawFrame(Renderer* renderer, float alpha)
{
    assert(renderer);

    renderer->statsDrawn = 0;
    renderer->statsCulled = 0;
    renderer->drawCount = 0;

    /* ── Step 1: Build draw list (Task 1.3 will add frustum culling here) ── */
    for (int i = 0; i < MAX_RENDERABLES; i++) 
    {
        if (!renderer->renderables[i].active) continue;

        /* TODO Task 1.3: frustum cull check here.
         * For now, everything passes. */

        renderer->drawList[renderer->drawCount].index = i;
        renderer->drawList[renderer->drawCount].distSq = 0.0f; /* TODO: compute */
        renderer->drawCount++;
    }

    /* TODO Task 1.4: sort drawList by materialID here. */

    /* ── Step 2: Draw ────────────────────────────────────────────────────── */
    ClearBackground(renderer->clearColor);
    BeginMode3D(renderer->camera);

    for (int d = 0; d < renderer->drawCount; d++) 
    {
        int idx = renderer->drawList[d].index;
        Renderable* r = &renderer->renderables[idx];

        /* Interpolate transform between prev and curr */
        Matrix interp = LerpMatrix(r->transformPrev, r->transformCurr, alpha);

        /* Apply interpolated transform to model and draw */
        r->model.transform = interp;
        DrawModel(r->model, (Vector3) { 0, 0, 0 }, 1.0f, WHITE);

        renderer->statsDrawn++;
    }

    /* Grid for reference (will be replaced by arena floor) */
    DrawGrid(20, 1.0f);

    EndMode3D();
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Camera Control
 * ═══════════════════════════════════════════════════════════════════════════ */

void RENDERER_SetCamera(Renderer* renderer, Camera3D camera)
{
    assert(renderer);
    renderer->camera = camera;
}

Camera3D RENDERER_GetCamera(const Renderer* renderer)
{
    assert(renderer);
    return renderer->camera;
}

void RENDERER_SetClearColor(Renderer* renderer, Color color)
{
    assert(renderer);
    renderer->clearColor = color;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Bounding Sphere Computation
 *
 *  Computes a bounding sphere that encloses all meshes in a model.
 *  Uses the AABB of each mesh to find the overall center, then finds
 *  the maximum distance from that center to any AABB corner.
 * ═══════════════════════════════════════════════════════════════════════════ */

void RENDERER_ComputeBoundingSphere(Model model,
    Vector3* outCenter, float* outRadius)
{
    assert(outCenter && outRadius);

    if (model.meshCount == 0) 
    {
        *outCenter = (Vector3){ 0, 0, 0 };
        *outRadius = 1.0f;  /* Default radius so culling doesn't reject it */
        return;
    }

    /* Compute combined AABB across all meshes */
    BoundingBox combined = GetMeshBoundingBox(model.meshes[0]);
    for (int i = 1; i < model.meshCount; i++) 
    {
        BoundingBox mb = GetMeshBoundingBox(model.meshes[i]);
        if (mb.min.x < combined.min.x) combined.min.x = mb.min.x;
        if (mb.min.y < combined.min.y) combined.min.y = mb.min.y;
        if (mb.min.z < combined.min.z) combined.min.z = mb.min.z;
        if (mb.max.x > combined.max.x) combined.max.x = mb.max.x;
        if (mb.max.y > combined.max.y) combined.max.y = mb.max.y;
        if (mb.max.z > combined.max.z) combined.max.z = mb.max.z;
    }

    /* Center = midpoint of combined AABB */
    outCenter->x = (combined.min.x + combined.max.x) * 0.5f;
    outCenter->y = (combined.min.y + combined.max.y) * 0.5f;
    outCenter->z = (combined.min.z + combined.max.z) * 0.5f;

    /* Radius = distance from center to the farthest corner */
    float dx = combined.max.x - outCenter->x;
    float dy = combined.max.y - outCenter->y;
    float dz = combined.max.z - outCenter->z;
    *outRadius = sqrtf(dx * dx + dy * dy + dz * dz);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Frustum Culling (Task 1.3 — stub for now)
 * ═══════════════════════════════════════════════════════════════════════════ */

void RENDERER_ExtractFrustumPlanes(FrustumPlane planes[6], Matrix viewProj)
{
    (void)planes;
    (void)viewProj;
    /* TODO Task 1.3: Extract 6 planes using Gribb-Hartmann method */
}

bool RENDERER_IsSphereInFrustum(const FrustumPlane planes[6],
    Vector3 center, float radius)
{
    (void)planes;
    (void)center;
    (void)radius;
    /* TODO Task 1.3: Test sphere against all 6 planes */
    return true;  /* Default: everything is visible */
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Screen Projection
 * ═══════════════════════════════════════════════════════════════════════════ */

Vector2 RENDERER_WorldToScreen(const Renderer* renderer, Vector3 worldPos)
{
    assert(renderer);
    return GetWorldToScreen(worldPos, renderer->camera);
}