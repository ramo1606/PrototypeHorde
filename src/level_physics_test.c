/*
 * level_physics_test.c — Stress test for physics subsystem.
 *
 * Exercises: the 6 collider shape combinations (box/capsule/sphere),
 * trigger volumes, and raycast queries. One controllable + one static
 * collider per shape; TAB cycles which one is active, WASD moves it.
 * Active body shoots a ray forward; ray segment + hit point drawn.
 *
 * Uses raylib third-person camera (CAMERA_THIRD_PERSON) targeting the
 * active controllable.
 */

#include "level_physics_test.h"
#include "level_manager.h"
#include "renderer.h"
#include "camera.h"
#include "arena.h"
#include "layers.h"
#include "physics.h"
#include "game.h"
#include "raymath.h"

#define MOVE_SPEED          4.0f
#define RAY_MAX_DIST        15.0f

#define BOX_HALF            0.5f    /* AABB halfExtents en todos los ejes */
#define SPHERE_RADIUS       0.5f
#define CAPSULE_RADIUS      0.35f
#define CAPSULE_HALFHEIGHT  0.4f    /* total altura = 2*0.4 + 2*0.35 = 1.5 */

#define TRIGGER_HALF_X      1.5f
#define TRIGGER_HALF_Y      1.0f
#define TRIGGER_HALF_Z      1.5f

#define BOX_Y      (BOX_HALF)                                  /* 0.5  */
#define CAPSULE_Y  (CAPSULE_HALFHEIGHT + CAPSULE_RADIUS)       /* 0.75 */
#define SPHERE_Y   (SPHERE_RADIUS)                             /* 0.5  */

enum
{
    SHAPE_BOX = 0,
    SHAPE_SPHERE = 1,
    SHAPE_CAPSULE = 2,
    SHAPE_COUNT = 3
};

static const Vector3 TRIGGER_POS = { 8.0f, TRIGGER_HALF_Y, 0.0f };

static const Vector2 CONTROLLABLE_XZ[SHAPE_COUNT] = {
    { -4.0f, -3.0f },   /* Box    */
    {  4.0f, -3.0f },   /* Sphere */
    {  0.0f, -3.0f },   /* Capsule */
};

static const Vector2 STATIC_XZ[SHAPE_COUNT] = {
    { -4.0f, 3.0f },    /* Box    */
    {  4.0f, 3.0f },    /* Sphere */
    {  0.0f, 3.0f },    /* Capsule */
};

static const float SHAPE_Y[SHAPE_COUNT] = {
    BOX_Y,
    SPHERE_Y,
    CAPSULE_Y,
};

static const Color CONTROLLABLE_COLORS[SHAPE_COUNT] = {
    { 240, 150, 60, 255 },   /* orange  */
    { 200, 100, 200, 255 },  /* magenta */
    { 240, 200, 80, 255 },   /* yellow  */
};

static const Color STATIC_COLOR = { 100, 110, 130, 255 };   /* slate gray */

typedef struct 
{
    ColliderHandle collider;
    RenderHandle render;
    Model model;
} TestBody;

typedef struct
{
    Camera3D camera;

    TestBody controllable[SHAPE_COUNT];
    TestBody staticBodies[SHAPE_COUNT];

    ColliderHandle  triggerCollider;

    int             activeIdx;
    bool            inTrigger;
    CollisionInfo   lastRayHit;
} PhysicsTestData;

static PhysicsTestData* data = NULL;

static void registerBody(Game* game, TestBody* body, ColliderType type, Vector3 pos, int layer, int mask, bool dynamic, Color color)
{
    switch (type) 
    {
        case COLLIDER_AABB:
            body->collider = PhysicsAddBox(&game->physWorld, pos, (Vector3) { BOX_HALF, BOX_HALF, BOX_HALF }, layer, mask, -1, dynamic, false);
            body->model = LoadModelFromMesh(GenMeshCube(BOX_HALF * 2.0f, BOX_HALF * 2.0f, BOX_HALF * 2.0f));
            break;
        case COLLIDER_SPHERE:
            body->collider = PhysicsAddSphere(&game->physWorld, pos, SPHERE_RADIUS, layer, mask, -1, dynamic, false);
            body->model = LoadModelFromMesh(GenMeshSphere(SPHERE_RADIUS, 16, 16));
            break;
        case COLLIDER_CAPSULE:
            body->collider = PhysicsAddCapsule(&game->physWorld, pos, CAPSULE_RADIUS, CAPSULE_HALFHEIGHT, layer, mask, -1, dynamic, false);
            body->model = LoadModelFromMesh(GenMeshCylinder(CAPSULE_RADIUS, (CAPSULE_HALFHEIGHT + CAPSULE_RADIUS) * 2.0f, 16));
            break;
    }

    body->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = color;
    body->render = RendererRegister(&game->renderer, body->model, 0);
    
    Matrix transform = MatrixTranslate(pos.x, pos.y, pos.z);
    if (type == COLLIDER_CAPSULE) 
    {
        transform = MatrixTranslate(pos.x, pos.y - (CAPSULE_HALFHEIGHT + CAPSULE_RADIUS), pos.z);
    }
    RendererSetTransform(&game->renderer, body->render, transform);
}

static void Init(void* user)
{ 
    Game* game = (Game*)user;
    data = ARENA_ALLOC(&game->level, PhysicsTestData);

    data->activeIdx = SHAPE_BOX;
    data->inTrigger = false;
    data->lastRayHit.hit = false;
    
    /* Camera */
    data->camera = (Camera3D) {
        .position = { 0.0f, 4.0f, -8.0f },
        .target   = { 0.0f, CAPSULE_Y, -3.0f },
        .up       = { 0.0f, 1.0f, 0.0f },
        .fovy     = 60.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    /* Controllable + static bodies. */
    const int ctrlLayer = LAYER_PLAYER;
    const int ctrlMask = LAYER_SCENERY | LAYER_PLAYER | LAYER_TRIGGER;
    const int staticLayer = LAYER_SCENERY;
    const int staticMask = MASK_ALL;

    for(int i = 0; i < SHAPE_COUNT; i++) 
    {
        Vector3 ctrlPos = { CONTROLLABLE_XZ[i].x, SHAPE_Y[i], CONTROLLABLE_XZ[i].y };
        Vector3 staticPos = { STATIC_XZ[i].x, SHAPE_Y[i], STATIC_XZ[i].y };
        ColliderType type = (i == SHAPE_BOX) ? COLLIDER_AABB 
                            : (i == SHAPE_CAPSULE) ? COLLIDER_CAPSULE 
                                                   : COLLIDER_SPHERE;
        registerBody(game, &data->controllable[i], type, ctrlPos, ctrlLayer, ctrlMask, true, CONTROLLABLE_COLORS[i]);
        registerBody(game, &data->staticBodies[i], type, staticPos, staticLayer, staticMask, false, STATIC_COLOR);
    }

    /* Trigger volume */
    data->triggerCollider = PhysicsAddBox(&game->physWorld, TRIGGER_POS, (Vector3) { TRIGGER_HALF_X, TRIGGER_HALF_Y, TRIGGER_HALF_Z }, LAYER_TRIGGER, LAYER_PLAYER, -1, false, true);

    DisableCursor();
}

static Camera3D* GetCamera(void* user)
{
    Game* game = (Game*)user;

    Vector3 activePos = PhysicsGetPosition(&game->physWorld, data->controllable[data->activeIdx].collider);
    data->camera.target = activePos;
    UpdateCamera(&data->camera, CAMERA_THIRD_PERSON);
    return &data->camera;
}

static void Shutdown(void* user)
{ 
    Game* game = (Game*)user;

    for(int i = 0; i < SHAPE_COUNT; i++) 
    {
        PhysicsRemove(&game->physWorld, data->controllable[i].collider);
        PhysicsRemove(&game->physWorld, data->staticBodies[i].collider);
        RendererUnregister(&game->renderer, data->controllable[i].render);
        RendererUnregister(&game->renderer, data->staticBodies[i].render);
        UnloadModel(data->controllable[i].model);
        UnloadModel(data->staticBodies[i].model);
    }

    PhysicsRemove(&game->physWorld, data->triggerCollider);

    EnableCursor();
    data = NULL;
}
static void Update(void* user, float dt)        { (void)user; (void)dt; }
static void Render3D(void* user, float alpha)   { (void)user; (void)alpha; }
static void RenderHUD(void* user, float alpha)  { (void)user; (void)alpha; }

Level LEVEL_PHYSICS_TEST = {
    .name      = "PhysicsTest",
    .Init      = Init,
    .Shutdown  = Shutdown,
    .GetCamera = GetCamera,
    .Update    = Update,
    .Render3D  = Render3D,
    .RenderHUD = RenderHUD,
};
