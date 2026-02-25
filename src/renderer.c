#include "renderer.h"
#include "game.h"
#include "actor.h"
#include "component.h"
#include "mesh_component.h"
#include "collision.h"
#include "level.h"
#include "level_manager.h"
#include "debug.h"
#include <assert.h>

static void NormalizePlane(FrustumPlane* p)
{
    /*
     * Divide all four plane coefficients (a,b,c,d) by the magnitude of
     * the normal (a,b,c) so the plane equation is in Hessian normal form.
     * After normalisation, dot(normal, point) + d gives the signed
     * distance from point to the plane.
     */
    float len = sqrtf(p->a * p->a + p->b * p->b + p->c * p->c);
    if (len > 1e-8f)
    {
        float inv = 1.0f / len;
        p->a *= inv;
        p->b *= inv;
        p->c *= inv;
        p->d *= inv;
    }
}

void RENDERER_ExtractFrustumPlanes(FrustumPlane planes[6], Matrix vp)
{
    /*
     * Gribb-Hartmann frustum plane extraction.
     *
     * Each frustum plane is a linear combination of the rows of the
     * view-projection matrix.  In column-major Raylib layout the
     * "rows" correspond to the m0/m1/m2/m3 ... columns of the struct,
     * where mN+0, mN+4, mN+8, mN+12 form a logical row.
     *
     *   Left   =  row3 + row0  (vp.m3+vp.m0, vp.m7+vp.m4, ...)
     *   Right  =  row3 - row0
     *   Bottom =  row3 + row1
     *   Top    =  row3 - row1
     *   Near   =  row3 + row2
     *   Far    =  row3 - row2
     *
     * After extraction each plane is normalised so distance comparisons
     * are meaningful.
     *
     * Reference: "Fast Extraction of Viewing Frustum Planes from the
     * World-View-Projection Matrix" — Gribb & Hartmann, 2001.
     */
    /* Left:   row3 + row0 */
    planes[0] = (FrustumPlane){ vp.m3 + vp.m0, vp.m7 + vp.m4, vp.m11 + vp.m8,  vp.m15 + vp.m12 };
    /* Right:  row3 - row0 */
    planes[1] = (FrustumPlane){ vp.m3 - vp.m0, vp.m7 - vp.m4, vp.m11 - vp.m8,  vp.m15 - vp.m12 };
    /* Bottom: row3 + row1 */
    planes[2] = (FrustumPlane){ vp.m3 + vp.m1, vp.m7 + vp.m5, vp.m11 + vp.m9,  vp.m15 + vp.m13 };
    /* Top:    row3 - row1 */
    planes[3] = (FrustumPlane){ vp.m3 - vp.m1, vp.m7 - vp.m5, vp.m11 - vp.m9,  vp.m15 - vp.m13 };
    /* Near:   row3 + row2 */
    planes[4] = (FrustumPlane){ vp.m3 + vp.m2, vp.m7 + vp.m6, vp.m11 + vp.m10, vp.m15 + vp.m14 };
    /* Far:    row3 - row2 */
    planes[5] = (FrustumPlane){ vp.m3 - vp.m2, vp.m7 - vp.m6, vp.m11 - vp.m10, vp.m15 - vp.m14 };

    for (int i = 0; i < 6; i++)
    {
        NormalizePlane(&planes[i]);
    }
}

bool RENDERER_IsAABBInFrustum(const FrustumPlane planes[6], BoundingBox box)
{
    /*
     * Positive-vertex (p-vertex) AABB frustum test.
     *
     * For each plane, find the box corner that is most in the direction of
     * the plane normal (the "positive vertex").  If that corner is on the
     * negative side of the plane, the entire box is outside the frustum
     * and we can early-exit with false.
     *
     * This is O(6) per box and has no false negatives (all culled boxes
     * are truly outside).  It may have false positives (intersection
     * boxes that span a corner), but those are rare and acceptable.
     */
    for (int i = 0; i < 6; i++)
    {
        /* Find the "positive vertex" — corner most aligned with plane normal */
        Vector3 pv = {
            planes[i].a >= 0 ? box.max.x : box.min.x,
            planes[i].b >= 0 ? box.max.y : box.min.y,
            planes[i].c >= 0 ? box.max.z : box.min.z,
        };

        /* If positive vertex is behind the plane, the AABB is fully outside */
        float dist = planes[i].a * pv.x + planes[i].b * pv.y + planes[i].c * pv.z + planes[i].d;
        if (dist < 0) return false;
    }

    return true;
}

bool RENDERER_IsPointInFrustum(const FrustumPlane planes[6], Vector3 point)
{
    /*
     * Test a single point against all 6 planes.  Returns false as soon
     * as the point is found to be on the negative (outside) side of any
     * plane.
     */
    for (int i = 0; i < 6; i++)
    {
        float dist = planes[i].a * point.x + planes[i].b * point.y + planes[i].c * point.z + planes[i].d;
        if (dist < 0) return false;
    }
	return true;
}

bool RENDERER_IsSphereInFrustum(const FrustumPlane planes[6], Vector3 center, float radius)
{
    /*
     * Sphere frustum test: the sphere is outside the frustum if the
     * signed distance from its centre to any plane is less than -radius
     * (the entire sphere is beyond the plane).
     */
    for (int i = 0; i < 6; i++)
    {
        float dist = planes[i].a * center.x + planes[i].b * center.y + planes[i].c * center.z + planes[i].d;
        if (dist < -radius) return false;
    }
	return true;
}

Vector2 RENDERER_WorldToScreen(const Renderer* renderer, Vector3 worldPos)
{
    /*
     * Manual NDC → screen-space projection.
     *
     * Steps:
     *   1. Build view and projection matrices from the renderer camera.
     *   2. Transform worldPos into clip space via VP matrix.
     *   3. Perform perspective divide (clip.xy / clip.z) to get NDC.
     *   4. Remap NDC [-1,1] to screen [0, width/height].
     *
     * Returns (-1,-1) if the point is behind or on the camera plane
     * (clipSpacePos.z <= 0) to signal "off-screen behind camera".
     */
	assert(renderer != NULL);
    Matrix view = GetCameraMatrix(renderer->camera);
    float aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
    Matrix proj = MatrixPerspective(renderer->camera.fovy * DEG2RAD, aspect, RENDERER_NEAR_PLANE, RENDERER_FAR_PLANE);
    Vector3 clipSpacePos = Vector3Transform(worldPos, MatrixMultiply(view, proj));
    if (clipSpacePos.z <= 0)
    {
        return (Vector2){ -1, -1 }; // Behind camera
    }
    return (Vector2){
        (int)((clipSpacePos.x / clipSpacePos.z + 1) * 0.5f * SCREEN_WIDTH),
        (int)((1 - (clipSpacePos.y / clipSpacePos.z + 1) * 0.5f) * SCREEN_HEIGHT)
	};
}

bool RENDERER_IsOnScreen(const Renderer* renderer, Vector3 worldPos)
{
	assert(renderer != NULL);
    Vector2 screenPos = RENDERER_WorldToScreen(renderer, worldPos);
    return screenPos.x >= 0 && screenPos.x < SCREEN_WIDTH &&
		screenPos.y >= 0 && screenPos.y < SCREEN_HEIGHT;
}

static void RENDERER_BuildDrawList(Renderer* renderer)
{
    /*
     * Frustum cull all registered meshes and populate the draw list.
     *
     * For each visible, active mesh:
     *   1. Compute its world-space AABB via 8-corner AABB transform.
     *   2. Test against the 6 frustum planes (positive-vertex method).
     *   3. If visible, record it in drawList[] with squared distance from
     *      camera centre to AABB centre (for future depth sorting).
     *
     * statsCulled and statsDrawn are updated for debug display.
     */
    renderer->drawCount = 0;
    renderer->statsCulled = 0;
    renderer->statsDrawn = 0;

    Vector3 camPos = renderer->camera.position;

    for (int i = 0; i < renderer->meshCount; i++)
    {
        MeshComponent* mc = renderer->meshes[i];
        if (!mc->visible) continue;
        if (!mc->mesh || !mc->material) continue;

        Actor* owner = mc->scene.base.owner;
        if (owner->state != ACTOR_STATE_ACTIVE) continue;

        SCENE_COMPONENT_ComputeWorldTransform(&mc->scene);
        BoundingBox worldBB = COLLISION_TransformAABB(mc->localBB, mc->scene.worldTransform);

        if (!RENDERER_IsAABBInFrustum(renderer->frustum, worldBB))
        {
            renderer->statsCulled++;
            continue;
        }

        Vector3 center = {
            (worldBB.min.x + worldBB.max.x) * 0.5f,
            (worldBB.min.y + worldBB.max.y) * 0.5f,
            (worldBB.min.z + worldBB.max.z) * 0.5f,
        };
        float distSq = Vector3DistanceSqr(camPos, center);

        renderer->drawList[renderer->drawCount++] = (DrawEntry){ mc, distSq };
    }

    renderer->statsDrawn = renderer->drawCount;
}

static int CompareByMaterial(const void* a, const void* b)
{
    /*
     * Sort draw entries by material pointer (ascending address).
     * Grouping by material minimises GPU state changes (texture binds,
     * shader switches) when submitting draw calls sequentially.
     */
    const DrawEntry* da = (const DrawEntry*)a;
    const DrawEntry* db = (const DrawEntry*)b;
    if (da->mc->material < db->mc->material) return -1;
    if (da->mc->material > db->mc->material) return  1;
    return 0;
}

static void RENDERER_SortDrawList(Renderer* renderer)
{
    if (renderer->drawCount > 1)
    {
        qsort(renderer->drawList, renderer->drawCount, sizeof(DrawEntry), CompareByMaterial);
    }
}

static void RENDERER_DrawMeshes(Renderer* renderer)
{
    for (int i = 0; i < renderer->drawCount; i++)
    {
        MESH_COMPONENT_Draw(renderer->drawList[i].mc);
    }
}

void RENDERER_Init(Renderer* renderer)
{
    /*
     * Initialise with a default perspective camera positioned to show the
     * origin at a comfortable angle, a dark blue-grey clear colour, and
     * all counters zeroed.
     */
    assert(renderer != NULL);

    renderer->camera = (Camera3D){
        .position = (Vector3){ 15.0f, 12.0f, 15.0f },
        .target = (Vector3){ 0.0f, 0.0f, 0.0f },
        .up = (Vector3){ 0.0f, 1.0f, 0.0f },
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    renderer->clearColor = (Color){ 20, 20, 40, 255 };
    renderer->meshCount = 0;
    renderer->drawCount = 0;
    renderer->statsCulled = 0;
    renderer->statsDrawn = 0;

    for (int i = 0; i < RENDERER_MAX_MESHES; i++)
        renderer->meshes[i] = NULL;

    TraceLog(LOG_INFO, "RENDERER: Initialized");
}

void RENDERER_Shutdown(Renderer* renderer)
{
    assert(renderer != NULL);
    (void)renderer;

    TraceLog(LOG_INFO, "RENDERER: Shutdown");
}

void RENDERER_DrawFrame(Renderer* renderer, Game* game)
{
    /*
     * Full render pipeline for one frame.
     *
     * ── Step 1: Extract frustum planes ─────────────────────────────
     * Build the combined view-projection matrix and extract 6 frustum
     * planes using the Gribb-Hartmann method.
     *
     * ── Step 2: Build and sort the draw list ───────────────────────
     * Frustum-cull all registered meshes and sort survivors by material
     * to minimise GPU state changes.
     *
     * ── Step 3: 3D draw pass ────────────────────────────────────────
     * Draw all surviving meshes, then hand off to the active level for
     * any additional 3D overlays (debug geometry, etc.).
     *
     * ── Step 4: HUD and 2D overlays ────────────────────────────────
     * Draw the active level's HUD, control hints, pause overlay, and
     * the debug system's 2D output.
     *
     * ── Step 5: Level manager transition overlay ───────────────────
     * Draw any fade/wipe effects managed by the LevelManager.
     */
    assert(renderer != NULL);
    assert(game != NULL);

    Level* active = LEVEL_MGR_GetActiveLevel(&game->levelMgr);

    /* ── Step 1: Extract frustum planes ─────────────────────────── */
    {
        float aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
        Matrix view = GetCameraMatrix(renderer->camera);
        Matrix proj = MatrixPerspective(renderer->camera.fovy * DEG2RAD,
            aspect, RENDERER_NEAR_PLANE,
            RENDERER_FAR_PLANE);
        Matrix vp = MatrixMultiply(view, proj);
        RENDERER_ExtractFrustumPlanes(renderer->frustum, vp);
    }

    /* ── Step 2: Build and sort the draw list ────────────────────── */
    RENDERER_BuildDrawList(renderer);
    RENDERER_SortDrawList(renderer);

    BeginDrawing();
        ClearBackground(renderer->clearColor);

        /* ── 3D ── */
        BeginMode3D(renderer->camera);

            RENDERER_DrawMeshes(renderer);

            if (active && active->Render3D)
            {
                active->Render3D(game);
            }

        EndMode3D();

        {
            int y = 10;

            if (active && active->RenderHUD)
            {
                y = active->RenderHUD(game, y);
            }

            DrawText("  ESC - Quit   P - Pause   F1 - Debug",
                10, y, 16, (Color) { 120, 120, 120, 200 });
        }

        if (game->state == GAME_STATE_PAUSED)
        {
            const char* msg = "PAUSED";
            int w = MeasureText(msg, 60);
            DrawText(msg,
                SCREEN_WIDTH / 2 - w / 2,
                SCREEN_HEIGHT / 2 - 30,
                60, RED);
        }

        DEBUG_Render(game);

        LEVEL_MGR_Render(&game->levelMgr);

    EndDrawing();
}

void RENDERER_AddMesh(Renderer* renderer, MeshComponent* mc)
{
    assert(renderer != NULL && mc != NULL);

    if (renderer->meshCount >= RENDERER_MAX_MESHES)
    {
        TraceLog(LOG_WARNING, "RENDERER: Mesh list full (%d)", RENDERER_MAX_MESHES);
        return;
    }

    renderer->meshes[renderer->meshCount++] = mc;
}

void RENDERER_RemoveMesh(Renderer* renderer, MeshComponent* mc)
{
    /*
     * Swap-remove: replace the found entry with the last entry and
     * decrement count.  O(n) scan, O(1) remove.
     */
    assert(renderer != NULL && mc != NULL);

    for (int i = 0; i < renderer->meshCount; i++)
    {
        if (renderer->meshes[i] == mc)
        {
            renderer->meshes[i] = renderer->meshes[renderer->meshCount - 1];
            renderer->meshes[renderer->meshCount - 1] = NULL;
            renderer->meshCount--;
            return;
        }
    }
}

void RENDERER_SetCamera(Renderer* renderer, Camera3D camera)
{
    assert(renderer != NULL);
    renderer->camera = camera;
}

Camera3D RENDERER_GetCamera(const Renderer* renderer)
{
    assert(renderer != NULL);
    return renderer->camera;
}

void RENDERER_SetClearColor(Renderer* renderer, Color color)
{
    assert(renderer != NULL);
    renderer->clearColor = color;
}