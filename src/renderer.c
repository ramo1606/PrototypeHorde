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

/* ── Internal: build draw list ────────────────────────────────── */

static void RENDERER_BuildDrawList(Renderer* renderer)
{
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

        /* Compute world AABB from local BB + spatial transform */
        SCENE_COMPONENT_ComputeWorldTransform(&mc->scene);
        BoundingBox worldBB = COLLISION_TransformAABB(mc->localBB, mc->scene.worldTransform);

        /* Frustum test */
        if (!RENDERER_IsAABBInFrustum(renderer->frustum, worldBB))
        {
            renderer->statsCulled++;
            continue;
        }

        /* Compute distance to camera (for potential future sorting) */
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

/* Sort by material pointer to batch state changes */
static int CompareByMaterial(const void* a, const void* b)
{
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

/* Draw all entries in the draw list */
static void RENDERER_DrawMeshes(Renderer* renderer)
{
    for (int i = 0; i < renderer->drawCount; i++)
    {
        MESH_COMPONENT_Draw((Component*)renderer->drawList[i].mc);
    }
}

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

void RENDERER_Shutdown(Renderer* renderer)
{
    assert(renderer != NULL);
    (void)renderer;

    TraceLog(LOG_INFO, "RENDERER: Shutdown");
}

void RENDERER_DrawFrame(Renderer* renderer, Game* game)
{
    assert(renderer != NULL);
    assert(game != NULL);

    Level* active = LEVEL_MGR_GetActiveLevel(&game->levelMgr);

    /* Extract frustum planes from current camera BEFORE drawing */
    {
        float aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
        Matrix view = GetCameraMatrix(renderer->camera);
        Matrix proj = MatrixPerspective(renderer->camera.fovy * DEG2RAD,
            aspect, RENDERER_NEAR_PLANE,
            RENDERER_FAR_PLANE);
        Matrix vp = MatrixMultiply(view, proj);
        RENDERER_ExtractFrustumPlanes(renderer->frustum, vp);
    }

    /* Build and sort draw list */
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

void RENDERER_SetClearColor(Renderer* renderer, Color color)
{
    assert(renderer != NULL);
    renderer->clearColor = color;
}