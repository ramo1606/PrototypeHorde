/*
 * level_renderer_test.c — Stress test for renderer subsystem.
 *
 * Exercises: frustum culling (dense grid of cubes), material sorting
 * (interleaved materials), interpolation (moving objects updated at
 * fixed timestep, drawn with alpha).
 *
 * Uses raylib free camera (CAMERA_FREE). No physics colliders.
 */

#include "level_renderer_test.h"
#include "level_manager.h"
#include "arena.h"
#include "game.h"
#include "config.h"
#include "renderer.h"
#include "raylib.h"
#include "raymath.h"
#include <math.h>

#define GRID_SIDE       14
#define GRID_SPACING    4.0f
#define MATERIAL_COUNT  4
#define ORBITER_COUNT   6

static const Color materialColors[MATERIAL_COUNT] = {
    { 220,  60,  60, 255 },   /* red   */
    {  60, 200,  80, 255 },   /* green  */
    {  60, 120, 220, 255 },   /* blue   */
    { 230, 200,  60, 255 },   /* yellow */
};

typedef struct 
{
    float radius;
    float height;
    float angSpeed;     /* rad/s */
    float phase;        /* rad */
} OrbiterParams;

static const OrbiterParams orbiters[ORBITER_COUNT] = {
      { 3.0f,  1.5f,  1.2f,  0.0f         },
      { 5.0f,  2.5f, -0.8f,  1.0471975f   },  /* 60° */
      { 8.0f,  1.0f,  0.6f,  2.0943951f   },  /* 120° */
      { 10.0f, 3.5f, -0.4f,  3.1415927f   },  /* 180° */
      { 12.0f, 2.0f,  0.3f,  4.1887902f   },  /* 240° */
      { 15.0f, 4.0f, -0.2f,  5.2359878f   },  /* 300° */
};

typedef struct
{
    Camera3D camera;

    Model gridModels[MATERIAL_COUNT];
    RenderHandle gridHandles[GRID_SIDE * GRID_SIDE];

    Model orbiterModel;
    RenderHandle orbiterHandle[ORBITER_COUNT];
    float time;
} RendererTestData;

static RendererTestData* data = NULL;

static void Init(void* user)
{ 
    Game* game = (Game*)user;

    data = ARENA_ALLOC(&game->level, RendererTestData);
    
    /* Free camera. */
    data->camera = (Camera3D) {
        .position = { 20.0f, 25.0f, 20.0f },
        .target   = { 0.0f, 0.0f, 0.0f },
        .up       = { 0.0f, 1.0f, 0.0f },
        .fovy     = 60.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    /* Cubes Grid */
    for (int i = 0; i < MATERIAL_COUNT; ++i) 
    {
        Mesh cubeMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
        data->gridModels[i] = LoadModelFromMesh(cubeMesh);
        data->gridModels[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].color = materialColors[i];
    }

    /* Orbiter Model */
    data->orbiterModel = LoadModelFromMesh(GenMeshSphere(0.4f, 12, 12));
    data->orbiterModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;

    /* Grid of cubes */
    const float gridOrigin = -((GRID_SIDE - 1) * GRID_SPACING) * 0.5f;

    for (int z = 0; z < GRID_SIDE; ++z) 
    {
        for (int x = 0; x < GRID_SIDE; ++x) 
        {
            int materialIdx = (x + z) % MATERIAL_COUNT;
            int handleIdx = z * GRID_SIDE + x;

            data->gridHandles[handleIdx] = RendererRegister(&game->renderer, data->gridModels[materialIdx], materialIdx);
            float wx = gridOrigin + x * GRID_SPACING;
            float wz = gridOrigin + z * GRID_SPACING;

            RendererSetTransform(&game->renderer, data->gridHandles[handleIdx],
                MatrixTranslate(wx, 0.5f, wz));
        }
    }

    for (int i = 0; i < ORBITER_COUNT; ++i) 
    {
        data->orbiterHandle[i] = RendererRegister(&game->renderer, data->orbiterModel, MATERIAL_COUNT);
    }

    data->time = 0.0f;

    DisableCursor();
}

static Camera3D* GetCamera(void* user)
{
    (void)user;
    UpdateCamera(&data->camera, CAMERA_FREE);
    return &data->camera;
}

static void Shutdown(void* user)
{
    Game* game = (Game*)user;

    /* Cube Grid. */
    for (int i = 0; i < GRID_SIDE * GRID_SIDE; ++i) 
    {
        RendererUnregister(&game->renderer, data->gridHandles[i]);
    }

    for (int i = 0; i < ORBITER_COUNT; ++i) 
    {
        RendererUnregister(&game->renderer, data->orbiterHandle[i]);
    }
    
    for(int i = 0; i < MATERIAL_COUNT; ++i)
    {
        UnloadModel(data->gridModels[i]);
    }
    
    UnloadModel(data->orbiterModel);
    EnableCursor();
    data = NULL;
}

static void Update(void* user, float dt) 
{ 
    Game* game = (Game*)user;
    data->time += dt;

    for (int i = 0; i < ORBITER_COUNT; ++i) 
    {
        const OrbiterParams* p = &orbiters[i];
        float angle = p->phase + p->angSpeed * data->time;

        Matrix transform = MatrixTranslate(cosf(angle) * p->radius, p->height, sinf(angle) * p->radius);
        RendererSetTransform(&game->renderer, data->orbiterHandle[i], transform);
    }
}
static void Render3D(void* user, float alpha)
{ 
    (void)user;
    (void)alpha;
    DrawGrid(60, 1.0f);
}
static void RenderHUD(void* user, float alpha)
{ 
    (void)user;
    (void)alpha;

    DrawText("RENDERER TEST", 10, SCREEN_HEIGHT - 75, 18, LIGHTGRAY);
    DrawText("WASD + mouse: free camera", 10, SCREEN_HEIGHT - 55, 14, GRAY);
    DrawText("Shift: speed up", 10, SCREEN_HEIGHT - 40, 14, GRAY);
    DrawText("Left/Right arrows: switch level", 10, SCREEN_HEIGHT - 25, 14, GRAY);
}

Level LEVEL_RENDERER_TEST = {
    .name      = "RendererTest",
    .GetCamera = GetCamera,
    .Init      = Init,
    .Shutdown  = Shutdown,
    .Update    = Update,
    .Render3D  = Render3D,
    .RenderHUD = RenderHUD,
};
