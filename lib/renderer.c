#include "renderer.h"
#include "raymath.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ── Lifecycle ───────────────────────────────────────────────────────────── */

void RendererInit(Renderer* renderer)
{
    assert(renderer);
    memset(renderer, 0, sizeof(*renderer));
    TraceLog(LOG_INFO, "Renderer: Initialized (max %d renderables)", RENDERER_MAX_RENDERABLES);
}

void RendererShutdown(Renderer* renderer)
{
    assert(renderer);
    memset(renderer->renderables, 0, sizeof(renderer->renderables));
    renderer->renderableCount = 0;
    TraceLog(LOG_INFO, "Renderer: Shutdown");
}

/* ── Registration ────────────────────────────────────────────────────────── */

RenderHandle RendererRegister(Renderer* renderer, Model model, int materialID)
{
    assert(renderer);

    for (int i = 0; i < RENDERER_MAX_RENDERABLES; i++)
    {
        if (!renderer->renderables[i].active)
        {
            Renderable* r = &renderer->renderables[i];
            memset(r, 0, sizeof(*r));

            r->model         = model;
            r->materialID    = materialID;
            r->active        = true;
            r->transformCurr = MatrixIdentity();
            r->transformPrev = MatrixIdentity();

            RendererComputeBoundingSphere(model,
                &r->boundingCenter,
                &r->boundingRadius);

            renderer->renderableCount++;
            return i;
        }
    }

    TraceLog(LOG_WARNING, "Renderer: Pool full, cannot register (max %d)",
        RENDERER_MAX_RENDERABLES);
    return RENDER_HANDLE_INVALID;
}

void RendererUnregister(Renderer* renderer, RenderHandle handle)
{
    assert(renderer);
    if (handle == RENDER_HANDLE_INVALID) return;
    if (handle < 0 || handle >= RENDERER_MAX_RENDERABLES) return;
    if (!renderer->renderables[handle].active) return;

    renderer->renderables[handle].active = false;
    renderer->renderableCount--;
}

/* ── Transform Updates ───────────────────────────────────────────────────── */

void RendererSetTransform(Renderer* renderer, RenderHandle handle, Matrix transform)
{
    assert(renderer);
    if (handle < 0 || handle >= RENDERER_MAX_RENDERABLES) return;
    if (!renderer->renderables[handle].active) return;

    renderer->renderables[handle].transformCurr = transform;
}

void RendererPreUpdate(Renderer* renderer)
{
    assert(renderer);
    for (int i = 0; i < RENDERER_MAX_RENDERABLES; i++)
    {
        if (renderer->renderables[i].active)
        {
            renderer->renderables[i].transformPrev = renderer->renderables[i].transformCurr;
        }
    }
}

/* ── Interpolation Helper ────────────────────────────────────────────────── */
/* Element-wise lerp on the 16 matrix floats. Strictly wrong (correct is
 * decompose T/R/S, lerp T, slerp R, lerp S), but at 60 Hz the per-tick
 * angular delta is small enough that this is visually indistinguishable
 * and significantly cheaper. */
static Matrix lerpMatrix(Matrix a, Matrix b, float t)
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

/* ── Draw List Sort Comparator ───────────────────────────────────────────── */
/* Primary: materialID asc (groups by shader/texture).
 * Secondary: distSq asc (front-to-back within a material, helps early-Z). */
static int compareDrawEntries(const void* a, const void* b)
{
    const DrawEntry* ea = (const DrawEntry*)a;
    const DrawEntry* eb = (const DrawEntry*)b;

    if (ea->materialID != eb->materialID)
    {
        return (ea->materialID > eb->materialID) - (ea->materialID < eb->materialID);
    }
    if (ea->distSq < eb->distSq) return -1;
    if (ea->distSq > eb->distSq) return  1;
    return 0;
}

/* ── BuildDrawList ───────────────────────────────────────────────────────── */

void RendererBuildDrawList(Renderer* renderer, Camera3D camera, float alpha)
{
    assert(renderer);

    renderer->statsDrawn  = 0;
    renderer->statsCulled = 0;
    renderer->drawCount   = 0;

    /* Build view-projection and extract frustum planes (Gribb-Hartmann). */
    Matrix view = GetCameraMatrix(camera);
    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    Matrix proj = MatrixPerspective(
        camera.fovy * DEG2RAD,
        aspect,
        RENDERER_NEAR_PLANE,
        RENDERER_FAR_PLANE
    );
    Matrix viewProj = MatrixMultiply(view, proj);
    RendererExtractFrustumPlanes(renderer->frustum, viewProj);

    for (int i = 0; i < RENDERER_MAX_RENDERABLES; i++)
    {
        if (!renderer->renderables[i].active) continue;

        Renderable* r = &renderer->renderables[i];

        Matrix interp = lerpMatrix(r->transformPrev, r->transformCurr, alpha);

        /* World-space bounding sphere: transform center, scale radius by
         * the max axis scale (overestimates for non-uniform scale, accepted). */
        Vector3 worldCenter = Vector3Transform(r->boundingCenter, interp);

        float sx = sqrtf(interp.m0 * interp.m0 + interp.m1 * interp.m1 + interp.m2 * interp.m2);
        float sy = sqrtf(interp.m4 * interp.m4 + interp.m5 * interp.m5 + interp.m6 * interp.m6);
        float sz = sqrtf(interp.m8 * interp.m8 + interp.m9 * interp.m9 + interp.m10 * interp.m10);
        float maxScale = sx > sy ? (sx > sz ? sx : sz) : (sy > sz ? sy : sz);
        float worldRadius = r->boundingRadius * maxScale;

        if (!RendererIsSphereInFrustum(renderer->frustum, worldCenter, worldRadius))
        {
            renderer->statsCulled++;
            continue;
        }

        Vector3 toCamera = Vector3Subtract(camera.position, worldCenter);
        float distSq = Vector3DotProduct(toCamera, toCamera);

        DrawEntry* entry = &renderer->drawList[renderer->drawCount];
        entry->index      = i;
        entry->materialID = r->materialID;
        entry->distSq     = distSq;
        entry->transform  = interp;
        renderer->drawCount++;
    }

    if (renderer->drawCount > 1)
    {
        qsort(renderer->drawList, (size_t)renderer->drawCount,
            sizeof(DrawEntry), compareDrawEntries);
    }
}

/* ── Draw3D ──────────────────────────────────────────────────────────────── */

void RendererDraw3D(Renderer* renderer)
{
    assert(renderer);

    for (int d = 0; d < renderer->drawCount; d++)
    {
        DrawEntry* entry = &renderer->drawList[d];
        Renderable* r = &renderer->renderables[entry->index];

        r->model.transform = entry->transform;
        DrawModel(r->model, (Vector3) { 0, 0, 0 }, 1.0f, WHITE);

        renderer->statsDrawn++;
    }
}

/* ── Bounding Sphere ─────────────────────────────────────────────────────── */

void RendererComputeBoundingSphere(Model model, Vector3* outCenter, float* outRadius)
{
    assert(outCenter && outRadius);

    if (model.meshCount == 0)
    {
        *outCenter = (Vector3){ 0, 0, 0 };
        *outRadius = 1.0f;
        return;
    }

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

    outCenter->x = (combined.min.x + combined.max.x) * 0.5f;
    outCenter->y = (combined.min.y + combined.max.y) * 0.5f;
    outCenter->z = (combined.min.z + combined.max.z) * 0.5f;

    float dx = combined.max.x - outCenter->x;
    float dy = combined.max.y - outCenter->y;
    float dz = combined.max.z - outCenter->z;
    *outRadius = sqrtf(dx * dx + dy * dy + dz * dz);
}

/* ── Frustum Culling (Gribb-Hartmann) ────────────────────────────────────── */
/* Plane indices: 0=Left, 1=Right, 2=Bottom, 3=Top, 4=Near, 5=Far. */

static void normalizePlane(FrustumPlane* p)
{
    float len = sqrtf(p->a * p->a + p->b * p->b + p->c * p->c);
    if (len > 0.0f)
    {
        float inv = 1.0f / len;
        p->a *= inv; p->b *= inv; p->c *= inv; p->d *= inv;
    }
}

void RendererExtractFrustumPlanes(FrustumPlane planes[6], Matrix m)
{
    planes[0] = (FrustumPlane){ m.m3 + m.m0, m.m7 + m.m4, m.m11 + m.m8,  m.m15 + m.m12 };
    planes[1] = (FrustumPlane){ m.m3 - m.m0, m.m7 - m.m4, m.m11 - m.m8,  m.m15 - m.m12 };
    planes[2] = (FrustumPlane){ m.m3 + m.m1, m.m7 + m.m5, m.m11 + m.m9,  m.m15 + m.m13 };
    planes[3] = (FrustumPlane){ m.m3 - m.m1, m.m7 - m.m5, m.m11 - m.m9,  m.m15 - m.m13 };
    planes[4] = (FrustumPlane){ m.m3 + m.m2, m.m7 + m.m6, m.m11 + m.m10, m.m15 + m.m14 };
    planes[5] = (FrustumPlane){ m.m3 - m.m2, m.m7 - m.m6, m.m11 - m.m10, m.m15 - m.m14 };

    for (int i = 0; i < 6; i++) normalizePlane(&planes[i]);
}

bool RendererIsSphereInFrustum(const FrustumPlane planes[6],
                               Vector3 center, float radius)
{
    for (int i = 0; i < 6; i++)
    {
        float dist = planes[i].a * center.x
                   + planes[i].b * center.y
                   + planes[i].c * center.z
                   + planes[i].d;
        if (dist < -radius) return false;
    }
    return true;
}
