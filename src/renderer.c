#include "renderer.h"
#include "resource_manager.h"
#include "raylib.h"
#include "raymath.h"

#include <assert.h>
#include <stdlib.h>
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

    /* ── Cel shader setup (Task 1.5) ──────────────────────────────────── */
    /*
     * Load the shader via resource manager (path: assets/shaders/cel).
     * Then get uniform locations for our custom params, and tell raylib
     * where matModel lives so it auto-sets it per DrawModel call.
     */
    RESOURCE_Load(RES_SHADER_CEL);
    renderer->celShader = RESOURCE_GetShader(RES_SHADER_CEL);

    /* Tell raylib where the model matrix uniform is in our shader.
     * This makes raylib set matModel automatically before each draw. */
    renderer->celShader.locs[SHADER_LOC_MATRIX_MODEL] =
        GetShaderLocation(renderer->celShader, "matModel");

    /* Cache custom uniform locations */
    renderer->locLightDir = GetShaderLocation(renderer->celShader, "lightDir");
    renderer->locAmbient = GetShaderLocation(renderer->celShader, "ambient");
    renderer->locNumBands = GetShaderLocation(renderer->celShader, "numBands");

    /* Default lighting: light from upper-right-front */
    renderer->lightDir = Vector3Normalize((Vector3) { 0.5f, 1.0f, 0.3f });
    renderer->ambient = 0.2f;
    renderer->numBands = 3.0f;

    /* ── Blob shadow setup (Task 1.7) ─────────────────────────────────── */
    /*
     * Generate a radial gradient texture: dark center → transparent edges.
     * This is drawn on a flat plane beneath each entity to fake a shadow.
     */
    {
#define SHADOW_TEX_SIZE 64
        Image img = GenImageColor(SHADOW_TEX_SIZE, SHADOW_TEX_SIZE, BLANK);
        Color* pixels = (Color*)img.data;
        float half = SHADOW_TEX_SIZE * 0.5f;

        for (int y = 0; y < SHADOW_TEX_SIZE; y++)
        {
            for (int x = 0; x < SHADOW_TEX_SIZE; x++)
            {
                float dx = (x + 0.5f - half) / half;   /* -1 to +1 */
                float dy = (y + 0.5f - half) / half;
                float dist = sqrtf(dx * dx + dy * dy);  /* 0 at center, 1 at edge */

                /* Smooth falloff: fully opaque at center, transparent at edge */
                float alpha = 1.0f - dist;
                if (alpha < 0.0f) alpha = 0.0f;
                alpha *= alpha;  /* Quadratic falloff for softer edge */

                unsigned char a = (unsigned char)(alpha * 160.0f);  /* Max ~63% opaque */
                pixels[y * SHADOW_TEX_SIZE + x] = (Color){ 0, 0, 0, a };
            }
        }

        renderer->shadowTex = LoadTextureFromImage(img);
        UnloadImage(img);
#undef SHADOW_TEX_SIZE

        /* Create a 1×1 plane mesh, scaled per-shadow at draw time */
        Mesh planeMesh = GenMeshPlane(1.0f, 1.0f, 1, 1);
        renderer->shadowPlane = LoadModelFromMesh(planeMesh);

        /* Assign shadow texture to the plane's material (default shader) */
        renderer->shadowPlane.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture =
            renderer->shadowTex;
    }

    TraceLog(LOG_INFO, "RENDERER: Initialized (max %d renderables)", MAX_RENDERABLES);
}

void RENDERER_Shutdown(Renderer* renderer)
{
    assert(renderer);

    /* Unload blob shadow resources */
    UnloadModel(renderer->shadowPlane);
    UnloadTexture(renderer->shadowTex);

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

            /* Assign cel shader to all materials of this model.
             * Note: r->model.materials is a shared pointer — this also
             * affects the original model, which is desired since all
             * renderables should use cel shading. */
            for (int m = 0; m < r->model.materialCount; m++)
            {
                r->model.materials[m].shader = renderer->celShader;
            }

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

void RENDERER_SetBlobShadow(Renderer* renderer, RenderHandle handle,
    bool enabled, float radius)
{
    assert(renderer);
    if (handle < 0 || handle >= MAX_RENDERABLES) return;
    if (!renderer->renderables[handle].active) return;

    renderer->renderables[handle].blobShadow = enabled;
    renderer->renderables[handle].blobRadius = radius;
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
 *  Draw List Sort Comparator (Task 1.4)
 *
 *  Primary key:   materialID (ascending) — groups draw calls by material
 *                 to minimize GPU state changes (shader/texture swaps).
 *  Secondary key: distSq (ascending, front-to-back) — within the same
 *                 material, closer objects draw first so the GPU's early-Z
 *                 can reject occluded fragments without running the
 *                 fragment shader.
 * ═══════════════════════════════════════════════════════════════════════════ */

static int CompareDrawEntries(const void* a, const void* b)
{
    const DrawEntry* ea = (const DrawEntry*)a;
    const DrawEntry* eb = (const DrawEntry*)b;

    /* Primary: group by material */
    if (ea->materialID != eb->materialID) 
    {
        return (ea->materialID > eb->materialID) - (ea->materialID < eb->materialID);
    }

    /* Secondary: front-to-back within same material */
    if (ea->distSq < eb->distSq) return -1;
    if (ea->distSq > eb->distSq) return  1;
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Frame Drawing — Phase 1: Build Draw List
 *
 *  Iterates all active renderables, interpolates their transforms,
 *  applies frustum culling (Task 1.3), and builds the sorted draw list.
 *
 *  Call ONCE per frame, BEFORE BeginMode3D. The interpolated transforms
 *  are cached in the DrawEntry so Draw3D doesn't recompute them.
 *
 *  Task 1.3: Add frustum culling before adding to draw list.
 *  Task 1.4: Add material sorting of draw list after building it.
 * ═══════════════════════════════════════════════════════════════════════════ */
void RENDERER_BuildDrawList(Renderer* renderer, float alpha)
{
    assert(renderer);

    renderer->statsDrawn = 0;
    renderer->statsCulled = 0;
    renderer->drawCount = 0;

    /* ── Extract frustum planes from the current camera ───────────────── */
    /*
     * Build the view-projection matrix:
     *   view = GetCameraMatrix (raylib's camera → view transform)
     *   proj = perspective from fovy + aspect + near/far
     *   viewProj = view * proj
     *
     * Gribb-Hartmann extracts the 6 frustum planes from viewProj.
     */
    Matrix view = GetCameraMatrix(renderer->camera);
    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    Matrix proj = MatrixPerspective(
        renderer->camera.fovy * DEG2RAD,
        aspect,
        RENDERER_NEAR_PLANE,
        RENDERER_FAR_PLANE
    );
    Matrix viewProj = MatrixMultiply(view, proj);
    RENDERER_ExtractFrustumPlanes(renderer->frustum, viewProj);

    /* ── Build draw list with frustum culling ─────────────────────────── */
    for (int i = 0; i < MAX_RENDERABLES; i++)
    {
        if (!renderer->renderables[i].active) continue;

        Renderable* r = &renderer->renderables[i];

        /* Interpolate transform between prev and curr */
        Matrix interp = LerpMatrix(r->transformPrev, r->transformCurr, alpha);

        /*
         * Transform bounding sphere to world space:
         *   - Center: apply the interpolated transform
         *   - Radius: scale by the max axis scale factor of the transform
         *
         * Extracting the scale: each column of the 3x3 rotation part has
         * length equal to the scale on that axis. We take the max to be
         * conservative — this overestimates for non-uniform scale, which
         * means we might not cull something we could, but we'll never
         * cull something visible.
         */
        Vector3 worldCenter = Vector3Transform(r->boundingCenter, interp);

        float sx = sqrtf(interp.m0 * interp.m0 + interp.m1 * interp.m1 + interp.m2 * interp.m2);
        float sy = sqrtf(interp.m4 * interp.m4 + interp.m5 * interp.m5 + interp.m6 * interp.m6);
        float sz = sqrtf(interp.m8 * interp.m8 + interp.m9 * interp.m9 + interp.m10 * interp.m10);
        float maxScale = sx > sy ? (sx > sz ? sx : sz) : (sy > sz ? sy : sz);
        float worldRadius = r->boundingRadius * maxScale;

        /* Frustum test */
        if (!RENDERER_IsSphereInFrustum(renderer->frustum, worldCenter, worldRadius))
        {
            renderer->statsCulled++;
            continue;
        }

        /* Passed culling — compute distance to camera for sorting */
        Vector3 toCamera = Vector3Subtract(renderer->camera.position, worldCenter);
        float distSq = Vector3DotProduct(toCamera, toCamera);

         /* Add to draw list with cached interpolated transform */
        DrawEntry* entry = &renderer->drawList[renderer->drawCount];
        entry->index = i;
        entry->distSq = 0.0f; /* TODO Task 1.4: compute distance to camera */
        entry->transform = interp;
        renderer->drawCount++;
    }

    /* Sort: group by material, then front-to-back within each group */
    if (renderer->drawCount > 1)
    {
        qsort(renderer->drawList, (size_t)renderer->drawCount,
            sizeof(DrawEntry), CompareDrawEntries);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Frame Drawing — Phase 2: Draw 3D
 *
 *  Submits draw calls for everything in the draw list.
 *  MUST be called INSIDE an active BeginMode3D/EndMode3D block —
 *  the renderer does NOT manage the 3D context.
 * ═══════════════════════════════════════════════════════════════════════════ */
void RENDERER_Draw3D(Renderer* renderer)
{
    assert(renderer);

    /*
     * Set cel shader uniforms once per frame.
     * These are global (same for all objects): light direction, ambient,
     * and band count. Per-object uniforms (matModel) are set automatically
     * by raylib because we registered SHADER_LOC_MATRIX_MODEL in Init.
     */
    float lightDirArr[3] = { renderer->lightDir.x, renderer->lightDir.y, renderer->lightDir.z };
    SetShaderValue(renderer->celShader, renderer->locLightDir, lightDirArr, SHADER_UNIFORM_VEC3);
    SetShaderValue(renderer->celShader, renderer->locAmbient, &renderer->ambient, SHADER_UNIFORM_FLOAT);
    SetShaderValue(renderer->celShader, renderer->locNumBands, &renderer->numBands, SHADER_UNIFORM_FLOAT);

    for (int d = 0; d < renderer->drawCount; d++)
    {
        DrawEntry* entry = &renderer->drawList[d];
        Renderable* r = &renderer->renderables[entry->index];

        /* Apply the pre-computed interpolated transform */
        r->model.transform = entry->transform;
        DrawModel(r->model, (Vector3) { 0, 0, 0 }, 1.0f, WHITE);

        renderer->statsDrawn++;
    }

    /* ── Blob shadow pass (Task 1.7) ──────────────────────────────────── */
    /*
     * For each visible renderable with blobShadow enabled, draw a
     * flat textured plane on the ground. The shadow scales down and
     * fades out as the entity gets higher (simulates jump/fall).
     *
     * We draw after models so depth testing handles occlusion naturally.
     * Alpha blending is needed because the shadow texture has transparency.
     */
    {
#define SHADOW_MAX_HEIGHT 6.0f      /* Height at which shadow disappears   */
#define SHADOW_GROUND_Y   0.01f     /* Slightly above ground to avoid z-fight */

        BeginBlendMode(BLEND_ALPHA);

        for (int d = 0; d < renderer->drawCount; d++)
        {
            DrawEntry* entry = &renderer->drawList[d];
            Renderable* r = &renderer->renderables[entry->index];

            if (!r->blobShadow) continue;

            /* Extract world position from the cached interpolated transform */
            float wx = entry->transform.m12;
            float wy = entry->transform.m13;
            float wz = entry->transform.m14;

            /* Height above ground (assume ground at Y=0) */
            float height = wy;
            if (height < 0.0f) height = 0.0f;
            if (height > SHADOW_MAX_HEIGHT) continue;  /* Too high → no shadow */

            /* Scale: shrinks as entity rises */
            float heightRatio = height / SHADOW_MAX_HEIGHT;
            float scale = r->blobRadius * 2.0f * (1.0f - heightRatio * 0.5f);

            /* Alpha: fades as entity rises */
            unsigned char alpha = (unsigned char)(255.0f * (1.0f - heightRatio));

            /* Build shadow transform: position at ground, scaled */
            renderer->shadowPlane.transform = MatrixMultiply(
                MatrixScale(scale, 1.0f, scale),
                MatrixTranslate(wx, SHADOW_GROUND_Y, wz)
            );

            DrawModel(renderer->shadowPlane, (Vector3) { 0, 0, 0 }, 1.0f,
                (Color) {
                255, 255, 255, alpha
            });
        }

        EndBlendMode();

#undef SHADOW_MAX_HEIGHT
#undef SHADOW_GROUND_Y
    }
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
 *  Lighting (Cel Shader Parameters)
 *
 *  These control the cel shading appearance. Levels can call these to
 *  set different moods (e.g. top-down noon light vs low-angle sunset).
 *  Values are applied in Draw3D before the draw loop.
 * ═══════════════════════════════════════════════════════════════════════════ */

void RENDERER_SetLightDir(Renderer* renderer, Vector3 dir)
{
    assert(renderer);
    renderer->lightDir = Vector3Normalize(dir);
}

void RENDERER_SetAmbient(Renderer* renderer, float ambient)
{
    assert(renderer);
    renderer->ambient = ambient;
}

void RENDERER_SetNumBands(Renderer* renderer, float numBands)
{
    assert(renderer);
    renderer->numBands = numBands;
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
  *  Frustum Culling (Task 1.3)
  *
  *  Gribb-Hartmann method: extract the 6 frustum planes directly from the
  *  combined view-projection matrix. Each plane comes from adding or
  *  subtracting row 3 with rows 0, 1, or 2 of the matrix.
  *
  *  Raylib Matrix layout (row-major naming, column-major storage):
  *      Row 0: m0  m4  m8   m12
  *      Row 1: m1  m5  m9   m13
  *      Row 2: m2  m6  m10  m14
  *      Row 3: m3  m7  m11  m15
  *
  *  Plane indices: 0=Left, 1=Right, 2=Bottom, 3=Top, 4=Near, 5=Far
  *
  *  Reference: "Fast Extraction of Viewing Frustum Planes from the
  *  World-View-Projection Matrix" — Gil Gribb & Klaus Hartmann
  * ═══════════════════════════════════════════════════════════════════════════ */

static void NormalizePlane(FrustumPlane* p)
{
    float len = sqrtf(p->a * p->a + p->b * p->b + p->c * p->c);
    if (len > 0.0f)
    {
        float inv = 1.0f / len;
        p->a *= inv;
        p->b *= inv;
        p->c *= inv;
        p->d *= inv;
    }
}

void RENDERER_ExtractFrustumPlanes(FrustumPlane planes[6], Matrix m)
{
    /* Left:   row3 + row0 */
    planes[0] = (FrustumPlane){ m.m3 + m.m0, m.m7 + m.m4, m.m11 + m.m8,  m.m15 + m.m12 };
    /* Right:  row3 - row0 */
    planes[1] = (FrustumPlane){ m.m3 - m.m0, m.m7 - m.m4, m.m11 - m.m8,  m.m15 - m.m12 };
    /* Bottom: row3 + row1 */
    planes[2] = (FrustumPlane){ m.m3 + m.m1, m.m7 + m.m5, m.m11 + m.m9,  m.m15 + m.m13 };
    /* Top:    row3 - row1 */
    planes[3] = (FrustumPlane){ m.m3 - m.m1, m.m7 - m.m5, m.m11 - m.m9,  m.m15 - m.m13 };
    /* Near:   row3 + row2 */
    planes[4] = (FrustumPlane){ m.m3 + m.m2, m.m7 + m.m6, m.m11 + m.m10, m.m15 + m.m14 };
    /* Far:    row3 - row2 */
    planes[5] = (FrustumPlane){ m.m3 - m.m2, m.m7 - m.m6, m.m11 - m.m10, m.m15 - m.m14 };

    /*
     * Normalize all planes so that (a,b,c) is unit length.
     * This makes the signed distance test in IsSphereInFrustum
     * return distances in world units, which we need to compare
     * against the bounding sphere radius.
     */
    for (int i = 0; i < 6; i++)
    {
        NormalizePlane(&planes[i]);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Sphere-vs-Frustum Test
 *
 *  Returns true if the sphere is at least partially inside the frustum.
 *  For each plane, we compute the signed distance from the sphere center
 *  to the plane. If the distance is less than -radius for ANY plane,
 *  the sphere is entirely behind that plane (outside the frustum).
 *
 *  This is conservative: it may return true for spheres that are actually
 *  outside (false positives at frustum corners), but never returns false
 *  for visible spheres (no false negatives). This is the correct tradeoff
 *  for culling — drawing one extra is fine, missing one visible is not.
 * ═══════════════════════════════════════════════════════════════════════════ */
bool RENDERER_IsSphereInFrustum(const FrustumPlane planes[6],
    Vector3 center, float radius)
{
    for (int i = 0; i < 6; i++)
    {
        float dist = planes[i].a * center.x
            + planes[i].b * center.y
            + planes[i].c * center.z
            + planes[i].d;

        if (dist < -radius)
        {
            return false;   /* Entirely behind this plane → invisible */
        }
    }

    return true;    /* Passed all 6 planes → at least partially visible */
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Screen Projection
 * ═══════════════════════════════════════════════════════════════════════════ */

Vector2 RENDERER_WorldToScreen(const Renderer* renderer, Vector3 worldPos)
{
    assert(renderer);
    return GetWorldToScreen(worldPos, renderer->camera);
}