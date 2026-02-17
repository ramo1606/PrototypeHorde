#include "renderer.h"
#include "game.h"
#include "actor.h"
#include "component.h"
#include "mesh_component.h"
#include "level.h"
#include "level_manager.h"
#include "debug.h"
#include <assert.h>

static void RENDERER_DrawMeshComponents(Game* game)
{
    for (int i = 0; i < game->actorCount; i++)
    {
        Actor* actor = game->actors[i];
        if (actor->state != ACTOR_STATE_ACTIVE) continue;

        Component* comp = ACTOR_GetComponentOfType(actor, COMPONENT_MESH);
        if (comp)
        {
            MESH_COMPONENT_Draw(comp);
        }
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

    BeginDrawing();
        ClearBackground(renderer->clearColor);

        /* ── 3D ── */
        BeginMode3D(renderer->camera);

            RENDERER_DrawMeshComponents(game);

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