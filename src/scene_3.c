#include "scene.h"
#include "game.h"
#include "actor.h"
#include "component.h"
#include "mesh_component.h"
#include <stdlib.h>
#include <assert.h>
#include <math.h>

typedef struct 
{
    Component base;
    float angularSpeed;
} SpinComponent;

static void SpinUpdate(Component* self, float deltaTime) 
{
	assert(self != NULL);
    SpinComponent* s = (SpinComponent*)self;
    Quaternion inc = QuaternionFromAxisAngle(
        (Vector3) {
        0, 1, 0
    }, s->angularSpeed * deltaTime);
    ACTOR_SetRotation(self->owner,
        QuaternionMultiply(self->owner->rotation, inc));
}

static SpinComponent* SPIN_COMPONENT_Create(Actor* owner, float speed) 
{
    SpinComponent* s = malloc(sizeof(SpinComponent));
    if (!s) return NULL;
    COMPONENT_Init(&s->base, owner, COMPONENT_NONE, 100);
    s->base.onUpdate = SpinUpdate;
    s->angularSpeed = speed;
    return s;
}

typedef struct 
{
    Component base;
    float bobTime;
    float baseY;
    float amplitude;
    float frequency;
} BobComponent;

static void BobUpdate(Component* self, float deltaTime) 
{
	assert(self != NULL);
    BobComponent* b = (BobComponent*)self;
    b->bobTime += deltaTime;
    Vector3 pos = self->owner->position;
    pos.y = b->baseY + sinf(b->bobTime * b->frequency) * b->amplitude;
    ACTOR_SetPosition(self->owner, pos);
}

static BobComponent* BOB_COMPONENT_Create(Actor* owner, float base_y,
    float amp, float freq) 
{
	assert(owner != NULL);
    BobComponent* b = malloc(sizeof(BobComponent));
    if (!b) return NULL;
    COMPONENT_Init(&b->base, owner, COMPONENT_NONE, 50);
    b->base.onUpdate = BobUpdate;
    b->bobTime = 0; b->baseY = base_y;
    b->amplitude = amp; b->frequency = freq;
    return b;
}

enum 
{ 
    MESH_CUBE = 0, 
    MESH_SPHERE, 
    MESH_CYLINDER, 
    MESH_FLOOR, 
    MESH_COUNT 
};

static Mesh     SMeshes[MESH_COUNT];
static Material SMaterial;

static void LoadResources(void) 
{
    SMeshes[MESH_CUBE] = GenMeshCube(1, 1, 1);
    SMeshes[MESH_SPHERE] = GenMeshSphere(0.5f, 16, 16);
    SMeshes[MESH_CYLINDER] = GenMeshCylinder(0.4f, 1, 16);
    SMeshes[MESH_FLOOR] = GenMeshPlane(20, 20, 1, 1);
    SMaterial = LoadMaterialDefault();
}

static void UnloadResources(void) 
{
    for (int i = 0; i < MESH_COUNT; i++) 
    {
        UnloadMesh(SMeshes[i]);
    }
    UnloadMaterial(SMaterial);
}

extern Scene SCENE_4;

static void SCENE_3_Init(Game* game) 
{
	assert(game != NULL);
	SetRandomSeed(GetTime());
    LoadResources();

    /* Floor */
    {
        Actor* a = malloc(sizeof(Actor));
		if (!a) return;
        ACTOR_Init(a, game);
		a->position = (Vector3){ 0, -0.01, 0 };
        MeshComponent* mc = MESH_COMPONENT_Create(a, &SMeshes[MESH_FLOOR], &SMaterial);
		mc->tint = GRAY;
    }

    /* Static cubes sharing the same Mesh */
    for (int i = 0; i < 5; i++) 
    {
        Actor* a = malloc(sizeof(Actor));
        ACTOR_Init(a, game);
        ACTOR_SetPosition(a, (Vector3) { (float)(i * 3 - 6), 0.5f, 0 });
        MESH_COMPONENT_Create(a, &SMeshes[MESH_CUBE], &SMaterial);
    }

    /* Spinning sphere */
    {
        Actor* a = malloc(sizeof(Actor));
        ACTOR_Init(a, game);
        ACTOR_SetPosition(a, (Vector3) { 0, 1, 5 });
        ACTOR_SetScale(a, 2.0f);
        MeshComponent* mc = MESH_COMPONENT_Create(a, &SMeshes[MESH_SPHERE], &SMaterial);
		mc->tint = RED;
        SPIN_COMPONENT_Create(a, 1.5f);
    }

    /* Bobbing cylinder */
    {
        Actor* a = malloc(sizeof(Actor));
        ACTOR_Init(a, game);
        ACTOR_SetPosition(a, (Vector3) { 0, 1, -5 });
        ACTOR_SetScale(a, 1.5f);
        MeshComponent* mc = MESH_COMPONENT_Create(a, &SMeshes[MESH_CYLINDER], &SMaterial);
		mc->tint = SKYBLUE;
        BOB_COMPONENT_Create(a, 1.0f, 0.6f, 2.5f);
    }

    /* Spinning + bobbing cube (full composition) */
    {
        Actor* a = malloc(sizeof(Actor));
        ACTOR_Init(a, game);
        ACTOR_SetPosition(a, (Vector3) { 5, 1.5f, 0 });
        ACTOR_SetScale(a, 1.2f);
        MeshComponent* mc = MESH_COMPONENT_Create(a, &SMeshes[MESH_CUBE], &SMaterial);
        mc->tint = GREEN;
        SPIN_COMPONENT_Create(a, 2.0f);
        BOB_COMPONENT_Create(a, 1.5f, 0.8f, 2.0f);
    }
}

static void SCENE_3_Shutdown(Game* game) 
{
	assert(game != NULL);
    (void)game;
    UnloadResources();
}

static void SCENE_3_Input(Game* game) 
{
	assert(game != NULL);
    if (game->state != GAME_STATE_GAMEPLAY) return;

    if (IsKeyPressed(KEY_SPACE)) 
    {
        Actor* a = malloc(sizeof(Actor));
        if (a) 
        {
            ACTOR_Init(a, game);
            float x = (float)GetRandomValue(-8, 7);
            float z = (float)GetRandomValue(-8, 7);
            ACTOR_SetPosition(a, (Vector3) { x, 0.5f, z });
            ACTOR_SetScale(a, 0.6f);
            Color colors[] = { RED, ORANGE, YELLOW, GREEN, SKYBLUE, PURPLE };
            MeshComponent* mc = MESH_COMPONENT_Create(a, &SMeshes[MESH_CUBE], &SMaterial);
			mc->tint = colors[GetRandomValue(0, 5)];
            SPIN_COMPONENT_Create(a, 1.0f + (float)GetRandomValue(0, 30) / 10.0f);
        }
    }

    if (IsKeyPressed(KEY_S)) 
    {
        Actor* a = malloc(sizeof(Actor));
        if (a) 
        {
            ACTOR_Init(a, game);
            float x = (float)GetRandomValue(-8, 7);
            float z = (float)GetRandomValue(-8, 7);
            ACTOR_SetPosition(a, (Vector3) { x, 0.8f, z });
            MeshComponent* mc = MESH_COMPONENT_Create(a, &SMeshes[MESH_SPHERE], &SMaterial);
			mc->tint = ORANGE;
            BOB_COMPONENT_Create(a, 0.8f, 0.4f, 3.0f);
        }
    }

    if (IsKeyPressed(KEY_V) && game->actorCount > 1) 
    {
        Actor* last = game->actors[game->actorCount - 1];
        Component* comp = ACTOR_GetComponentOfType(last, COMPONENT_MESH);
        if (comp) 
        {
            MeshComponent* mc = (MeshComponent*)comp;
			mc->visible = !mc->visible;
        }
    }

    if (IsKeyPressed(KEY_BACKSPACE) && game->actorCount > 1) 
    {
        game->actors[game->actorCount - 1]->state = ACTOR_STATE_DEAD;
    }

    if (IsKeyPressed(KEY_TAB)) 
    {
        GAME_ChangeScene(game, &SCENE_4);
    }
}

static void SCENE_3_Render3D(Game* game) 
{
	assert(game != NULL);
    DrawGrid(20, 1.0f);
    for (int i = 0; i < game->actorCount; i++)
    {
        Actor* a = game->actors[i];
        if (a->state != ACTOR_STATE_ACTIVE) continue;
        if (!ACTOR_GetComponentOfType(a, COMPONENT_MESH)) continue;

        Vector3 fwd = ACTOR_GetForward(a);
        DrawLine3D(a->position,
            Vector3Add(a->position, Vector3Scale(fwd, 1.5f)),
            YELLOW);
    }
}

static int SCENE_3_RenderHUD(Game* game, int y) 
{
	assert(game != NULL);
    const int step = 22;
    (void)game;

    DrawText("Scene 3 - MeshComponent", 10, y, 18, WHITE);
    y += step + 4;

    DrawText("SPACE - Spawn cube   S - Spawn sphere   V - Toggle vis", 10, y, 16, LIGHTGRAY);
    y += step;
    DrawText("BACKSPACE - Kill last", 10, y, 16, LIGHTGRAY);
    y += step;
    DrawText("TAB - Next scene", 10, y, 16, YELLOW);
    y += step;

    return y;
}

Scene SCENE_3 = 
{
    .name = "3 - MeshComponent",
    .SCENE_Init = SCENE_3_Init,
    .SCENE_Shutdown = SCENE_3_Shutdown,
    .SCENE_ProcessInput = SCENE_3_Input,
    .SCENE_Render3D = SCENE_3_Render3D,
    .SCENE_RenderHUD = SCENE_3_RenderHUD,
};