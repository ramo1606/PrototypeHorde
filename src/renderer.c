/*******************************************************************************************
*
*   renderer.c — Renderer Implementation
*
********************************************************************************************/
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

/* ═══════════════════════════════════════════════════════════════════════════
 *  Frustum Plane Extraction — Gribb-Hartmann Method
 * 
 *  Given a combined View × Projection matrix, the 6 frustum planes can be
 *  extracted directly from the matrix rows. Each plane is a linear combination
 *  of row 3 ± another row.
 * 
 *  After extraction, planes are normalized so that (a,b,c) is a unit normal.
 *  This makes distance calculations (ax + by + cz + d) return true distances.
 * 
 *  Reference: "Fast Extraction of Viewing Frustum Planes from the 
 *              World-View-Projection Matrix" — Gil Gribb & Klaus Hartmann
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Normalize a frustum plane so that (a,b,c) is unit length. */
static void NormalizePlane(FrustumPlane* p)
{
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

/*------------------------------------------------------------------------------------
 * RENDERER_ExtractFrustumPlanes
 * 
 *   Extracts 6 frustum planes from the View-Projection matrix.
 * 
 *   Plane derivation (Raylib column-major convention):
 *       Left   = row3 + row0     Right  = row3 - row0
 *       Bottom = row3 + row1     Top    = row3 - row1
 *       Near   = row3 + row2     Far    = row3 - row2
 * 
 *   Each "row" is extracted from the column-major matrix using the appropriate
 *   m-indices. All 6 planes are normalized after extraction.
 *------------------------------------------------------------------------------------*/
void RENDERER_ExtractFrustumPlanes(FrustumPlane planes[6], Matrix vp)
{
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

/* ═══════════════════════════════════════════════════════════════════════════
 *  Frustum Culling Tests
 * 
 *  For each of the 6 planes, we test if the object is entirely behind it.
 *  If it's behind ANY plane, it's outside the frustum → cull it.
 * 
 *  "Behind" means the signed distance to the plane is negative.
 *  The signed distance of a point P to plane (a,b,c,d) is: a*Px + b*Py + c*Pz + d
 * ═══════════════════════════════════════════════════════════════════════════ */

/*------------------------------------------------------------------------------------
 * RENDERER_IsAABBInFrustum
 * 
 *   Tests if an AABB is at least partially inside the frustum.
 * 
 *   Algorithm: Positive Vertex Test
 *   For each plane, find the AABB corner most aligned with the plane's normal
 *   (the "positive vertex" or "P-vertex"). This is the corner that would be
 *   farthest in the direction of the normal:
 *       pv.x = (normal.x >= 0) ? box.max.x : box.min.x
 *       pv.y = (normal.y >= 0) ? box.max.y : box.min.y
 *       pv.z = (normal.z >= 0) ? box.max.z : box.min.z
 * 
 *   If the P-vertex is behind the plane (negative distance), then ALL of the
 *   AABB is behind that plane → the AABB is outside the frustum.
 * 
 *   This test is conservative: it may return true for boxes that are technically
 *   outside (near frustum corners), but never false for visible boxes.
 * 
 *   Reference: "Optimized View Frustum Culling Algorithms" — Ulf Assarsson & Tomas Möller
 *------------------------------------------------------------------------------------*/
bool RENDERER_IsAABBInFrustum(const FrustumPlane planes[6], BoundingBox box)
{
    for (int i = 0; i < 6; i++)
    {
        /* Find the "positive vertex" — corner most aligned with plane normal */
        Vector3 pv = {
            planes[i].a >= 0 ? box.max.x : box.min.x,
            planes[i].b >= 0 ? box.max.y : box.min.y,
            planes[i].c >= 0 ? box.max.z : box.min.z,
        };

        /* If the P-vertex is behind the plane, the AABB is fully outside */
        float dist = planes[i].a * pv.x + planes[i].b * pv.y + planes[i].c * pv.z + planes[i].d;
        if (dist < 0) return false;
    }

    return true;
}

/* Test if a point is inside all 6 frustum planes. */
bool RENDERER_IsPointInFrustum(const FrustumPlane planes[6], Vector3 point)
{
    for (int i = 0; i < 6; i++)
    {
        float dist = planes[i].a * point.x + planes[i].b * point.y + planes[i].c * point.z + planes[i].d;
        if (dist < 0) return false;
    }
	return true;
}

/*------------------------------------------------------------------------------------
 * RENDERER_IsSphereInFrustum
 * 
 *   Tests if a sphere is at least partially inside the frustum.
 *   A sphere is outside if its center is more than 'radius' units behind any plane.
 *   (signed distance < -radius means the entire sphere is behind that plane)
 *------------------------------------------------------------------------------------*/
bool RENDERER_IsSphereInFrustum(const FrustumPlane planes[6], Vector3 center, float radius)
{
    for (int i = 0; i < 6; i++)
    {
        float dist = planes[i].a * center.x + planes[i].b * center.y + planes[i].c * center.z + planes[i].d;
        if (dist < -radius) return false;
    }
	return true;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Screen Projection
 * ═══════════════════════════════════════════════════════════════════════════ */

/*------------------------------------------------------------------------------------
 * RENDERER_WorldToScreen
 * 
 *   Projects a 3D world-space point to 2D screen coordinates.
 * 
 *   Algorithm:
 *   1. Multiply worldPos by View × Projection → clip space (x, y, z).
 *   2. If z <= 0, the point is behind the camera → return (-1,-1).
 *   3. Perspective divide: x/z, y/z → normalized device coordinates [-1,1].
 *   4. Map to screen pixels: (ndc + 1) * 0.5 * screenSize.
 *   5. Flip Y (screen Y increases downward, NDC Y increases upward).
 *   TODO: can I use GetWorldToScreen function family from raylib?
 *------------------------------------------------------------------------------------*/
Vector2 RENDERER_WorldToScreen(const Renderer* renderer, Vector3 worldPos)
{
	assert(renderer != NULL);
    Matrix view = GetCameraMatrix(renderer->camera);
    float aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
    Matrix proj = MatrixPerspective(renderer->camera.fovy * DEG2RAD, aspect, RENDERER_NEAR_PLANE, RENDERER_FAR_PLANE);

    /* Transform to clip space */
    Vector3 clipSpacePos = Vector3Transform(worldPos, MatrixMultiply(view, proj));

    /* Behind camera check */
    if (clipSpacePos.z <= 0)
    {
        return (Vector2){ -1, -1 }; // Behind camera
    }

    /* Perspective divide + map to screen pixels */
    return (Vector2){
        (int)((clipSpacePos.x / clipSpacePos.z + 1) * 0.5f * SCREEN_WIDTH),
        (int)((1 - (clipSpacePos.y / clipSpacePos.z + 1) * 0.5f) * SCREEN_HEIGHT)
	};
}

/* Check if a world point projects within the visible screen bounds. */
bool RENDERER_IsOnScreen(const Renderer* renderer, Vector3 worldPos)
{
	assert(renderer != NULL);
    Vector2 screenPos = RENDERER_WorldToScreen(renderer, worldPos);
    return screenPos.x >= 0 && screenPos.x < SCREEN_WIDTH &&
		screenPos.y >= 0 && screenPos.y < SCREEN_HEIGHT;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Draw List Management
 * 
 *  Each frame, the renderer builds a draw list from all registered meshes:
 *  1. Skip invisible, incomplete, or dead meshes
 *  2. Frustum-cull using the world AABB
 *  3. Record surviving meshes with their distance to camera
 *  4. Sort by material to minimize GPU state changes
 * ═══════════════════════════════════════════════════════════════════════════ */

/*------------------------------------------------------------------------------------
 * RENDERER_BuildDrawList (static)
 * 
 *   Iterates all registered meshes and builds the draw list:
 *   - Skips invisible meshes, meshes missing data, and dead actors
 *   - Computes world AABB using COLLISION_TransformAABB (correct under rotation)
 *   - Tests AABB against frustum planes
 *   - Records visible meshes with squared distance to camera (for potential
 *     distance-based sorting in the future)
 *------------------------------------------------------------------------------------*/
static void RENDERER_BuildDrawList(Renderer* renderer)
{
    renderer->drawCount = 0;
    renderer->statsCulled = 0;
    renderer->statsDrawn = 0;

    Vector3 camPos = renderer->camera.position;

    for (int i = 0; i < renderer->meshCount; i++)
    {
        /* ── Early rejection ── */
        MeshComponent* mc = renderer->meshes[i];
        if (!mc->visible) continue;
        if (!mc->mesh || !mc->material) continue;

        Actor* owner = mc->scene.base.owner;
        if (owner->state != ACTOR_STATE_ACTIVE) continue;

        /* ── Frustum culling ── */
        SCENE_COMPONENT_ComputeWorldTransform(&mc->scene);
        BoundingBox worldBB = COLLISION_TransformAABB(mc->localBB, mc->scene.worldTransform);

        if (!RENDERER_IsAABBInFrustum(renderer->frustum, worldBB))
        {
            renderer->statsCulled++;
            continue;
        }

        /* ── Record in draw list ── */
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

/*------------------------------------------------------------------------------------
 * CompareByMaterial (static)
 * 
 *   qsort comparator: sorts draw entries by material pointer address.
 *   Grouping same-material meshes together minimizes GPU state changes
 *   (shader binds, texture binds), which is one of the most impactful
 *   rendering optimizations.
 *------------------------------------------------------------------------------------*/
static int CompareByMaterial(const void* a, const void* b)
{
    const DrawEntry* da = (const DrawEntry*)a;
    const DrawEntry* db = (const DrawEntry*)b;
    if (da->mc->material < db->mc->material) return -1;
    if (da->mc->material > db->mc->material) return  1;
    return 0;
}

/* Sort the draw list by material pointer to minimize state changes. */
static void RENDERER_SortDrawList(Renderer* renderer)
{
    if (renderer->drawCount > 1)
    {
        qsort(renderer->drawList, renderer->drawCount, sizeof(DrawEntry), CompareByMaterial);
    }
}

/* Draw all meshes in the sorted draw list. */
static void RENDERER_DrawMeshes(Renderer* renderer)
{
    for (int i = 0; i < renderer->drawCount; i++)
    {
        MESH_COMPONENT_Draw(renderer->drawList[i].mc);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Lifecycle & Main Render Loop
 * ═══════════════════════════════════════════════════════════════════════════ */

/*------------------------------------------------------------------------------------
 * RENDERER_Init
 * 
 *   Sets up default camera, clear color, and zeroes all tracking arrays.
 *   Default camera is positioned at (15,12,15) looking at origin — good for
 *   debugging before a CameraComponent takes over.
 *------------------------------------------------------------------------------------*/
void RENDERER_Init(Renderer* renderer)
{
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

/* Shutdown — currently no resources to release beyond what Raylib handles. */
void RENDERER_Shutdown(Renderer* renderer)
{
    assert(renderer != NULL);
    (void)renderer;

    TraceLog(LOG_INFO, "RENDERER: Shutdown");
}

/*------------------------------------------------------------------------------------
 * RENDERER_DrawFrame
 * 
 *   Executes the complete render pipeline for one frame:
 * 
 *   1. FRUSTUM EXTRACTION
 *      Build View and Projection matrices from the active camera, multiply them
 *      to get VP, and extract 6 frustum planes using Gribb-Hartmann.
 * 
 *   2. DRAW LIST CONSTRUCTION
 *      Iterate all meshes, cull against frustum, build sorted draw list.
 * 
 *   3. RENDERING
 *      BeginDrawing → ClearBackground → BeginMode3D → draw meshes + level 3D →
 *      EndMode3D → draw HUD → draw pause overlay → draw debug → draw transitions →
 *      EndDrawing.
 * 
 *   The level's Render3D and RenderHUD callbacks allow each level to add custom
 *   drawing without modifying the renderer.
 *------------------------------------------------------------------------------------*/
void RENDERER_DrawFrame(Renderer* renderer, Game* game)
{
    assert(renderer != NULL);
    assert(game != NULL);

    Level* active = LEVEL_MGR_GetActiveLevel(&game->levelMgr);

    /* ── Step 1: Extract frustum planes from View × Projection ── */
    {
        float aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
        Matrix view = GetCameraMatrix(renderer->camera);
        Matrix proj = MatrixPerspective(renderer->camera.fovy * DEG2RAD,
            aspect, RENDERER_NEAR_PLANE,
            RENDERER_FAR_PLANE);
        Matrix vp = MatrixMultiply(view, proj);
        RENDERER_ExtractFrustumPlanes(renderer->frustum, vp);
    }

    /* ── Step 2: Build and sort draw list ── */
    RENDERER_BuildDrawList(renderer);
    RENDERER_SortDrawList(renderer);

    /* ── Step 3: Render frame ── */
    BeginDrawing();
        ClearBackground(renderer->clearColor);

        /* ── 3D Scene ── */
        BeginMode3D(renderer->camera);

            RENDERER_DrawMeshes(renderer);

            /* Level-specific 3D drawing (grid, gizmos, etc.) */
            if (active && active->Render3D)
            {
                active->Render3D(game);
            }

        EndMode3D();

        /* ── 2D Overlay / HUD ── */
        {
            int y = 10;

            if (active && active->RenderHUD)
            {
                y = active->RenderHUD(game, y);
            }

            DrawText("  ESC - Quit   P - Pause   F1 - Debug",
                10, y, 16, (Color) { 120, 120, 120, 200 });
        }

        /* Pause overlay */
        if (game->state == GAME_STATE_PAUSED)
        {
            const char* msg = "PAUSED";
            int w = MeasureText(msg, 60);
            DrawText(msg,
                SCREEN_WIDTH / 2 - w / 2,
                SCREEN_HEIGHT / 2 - 30,
                60, RED);
        }

        /* Debug overlay (memory stats, FPS, etc.) */
        DEBUG_Render(game);

        /* Level transition effects (fade, wipe) */
        LEVEL_MGR_Render(&game->levelMgr);

    EndDrawing();
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Mesh Registration — Swap-with-Last Pattern
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Register a mesh for rendering (called by MESH_COMPONENT_Create). */
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

/* Unregister a mesh (called by MeshComponentDestroy). Swap-with-last for O(1). */
void RENDERER_RemoveMesh(Renderer* renderer, MeshComponent* mc)
{
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

/* ═══════════════════════════════════════════════════════════════════════════
 *  Camera & Clear Color
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Set the active camera. Only one camera is active at a time. */
void RENDERER_SetCamera(Renderer* renderer, Camera3D camera)
{
    assert(renderer != NULL);
    renderer->camera = camera;
}

/* Get a copy of the current camera. */
Camera3D RENDERER_GetCamera(const Renderer* renderer)
{
    assert(renderer != NULL);
    return renderer->camera;
}

/* Set the clear color used at the start of each frame. */
void RENDERER_SetClearColor(Renderer* renderer, Color color)
{
    assert(renderer != NULL);
    renderer->clearColor = color;
}