#include "level.h"
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
    ACTOR_SetRotation(self->owner,
        Vector3Add(self->owner->root.rotation, (Vector3){ 0, s->angularSpeed * deltaTime, 0 }));
}

static SpinComponent* SPIN_COMPONENT_Create(Actor* owner, float speed) 
{
    SpinComponent* s = (SpinComponent*)MEMORY_AllocComponent(
        &owner->game->memory, sizeof(SpinComponent));
    if (!s) return NULL;
    COMPONENT_Init(&s->base, owner, COMPONENT_NONE, 100);
    s->base.Update = SpinUpdate;
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
    Vector3 pos = ACTOR_GetWorldPosition(self->owner);
    pos.y = b->baseY + sinf(b->bobTime * b->frequency) * b->amplitude;
    ACTOR_SetPosition(self->owner, pos);
}

static BobComponent* BOB_COMPONENT_Create(Actor* owner, float base_y,
    float amp, float freq) 
{
	assert(owner != NULL);
    BobComponent* b = (BobComponent*)MEMORY_AllocComponent(
        &owner->game->memory, sizeof(BobComponent));
    if (!b) return NULL;
    COMPONENT_Init(&b->base, owner, COMPONENT_NONE, 50);
    b->base.Update = BobUpdate;
    b->bobTime = 0; b->baseY = base_y;
    b->amplitude = amp; b->frequency = freq;
    return b;
}

enum
{
    L3_MESH_CUBE = 0,
    L3_MESH_SPHERE,
    L3_MESH_CYLINDER,
    L3_MESH_FLOOR,
    L3_L3_MESH_COUNT
};

static Mesh     SMeshes[L3_L3_MESH_COUNT];
static Material SMaterial;

static void LoadResources(void) 
{
    SMeshes[L3_MESH_CUBE] = GenMeshCube(1, 1, 1);
    SMeshes[L3_MESH_SPHERE] = GenMeshSphere(0.5f, 16, 16);
    SMeshes[L3_MESH_CYLINDER] = GenMeshCylinder(0.4f, 1, 16);
    SMeshes[L3_MESH_FLOOR] = GenMeshPlane(20, 20, 1, 1);
    SMaterial = LoadMaterialDefault();
}

static void UnloadResources(void) 
{
    for (int i = 0; i < L3_L3_MESH_COUNT; i++) 
    {
        UnloadMesh(SMeshes[i]);
    }
    UnloadMaterial(SMaterial);
}

extern Level LEVEL_4;

static void LEVEL_3_Init(Game* game) 
{
	assert(game != NULL);
	SetRandomSeed(GetTime());
    LoadResources();

    /* Floor */
    {
        Actor* a = ACTOR_Create(game);
		if (!a) return;
		ACTOR_SetPosition(a, (Vector3) { 0, -0.001f, 0 });
        MeshComponent* mc = MESH_COMPONENT_Create(a, &SMeshes[L3_MESH_FLOOR], &SMaterial);
		mc->tint = GRAY;
    }

    /* Static cubes sharing the same Mesh */
    for (int i = 0; i < 5; i++) 
    {
        Actor* a = ACTOR_Create(game);
        if (!a) return;
        ACTOR_SetPosition(a, (Vector3) { (float)(i * 3 - 6), 0.5f, 0 });
        MESH_COMPONENT_Create(a, &SMeshes[L3_MESH_CUBE], &SMaterial);
    }

    /* Spinning sphere */
    {
        Actor* a = ACTOR_Create(game);
        if (!a) return;
        ACTOR_SetPosition(a, (Vector3) { 0, 1, 5 });
        ACTOR_SetScale(a, 2.0f);
        MeshComponent* mc = MESH_COMPONENT_Create(a, &SMeshes[L3_MESH_SPHERE], &SMaterial);
		mc->tint = RED;
        SPIN_COMPONENT_Create(a, 1.5f);
    }

    /* Bobbing cylinder */
    {
        Actor* a = ACTOR_Create(game);
        if (!a) return;
        ACTOR_SetPosition(a, (Vector3) { 0, 1, -5 });
        ACTOR_SetScale(a, 1.5f);
        MeshComponent* mc = MESH_COMPONENT_Create(a, &SMeshes[L3_MESH_CYLINDER], &SMaterial);
		mc->tint = SKYBLUE;
        BOB_COMPONENT_Create(a, 1.0f, 0.6f, 2.5f);
    }

    /* Spinning + bobbing cube (full composition) */
    {
        Actor* a = ACTOR_Create(game);
        if (!a) return;
        ACTOR_SetPosition(a, (Vector3) { 5, 1.5f, 0 });
        ACTOR_SetScale(a, 1.2f);
        MeshComponent* mc = MESH_COMPONENT_Create(a, &SMeshes[L3_MESH_CUBE], &SMaterial);
        mc->tint = GREEN;
        SPIN_COMPONENT_Create(a, 2.0f);
        BOB_COMPONENT_Create(a, 1.5f, 0.8f, 2.0f);
    }
}

static void LEVEL_3_Shutdown(Game* game) 
{
	assert(game != NULL);
    (void)game;
    UnloadResources();
}

static void LEVEL_3_Input(Game* game) 
{
	assert(game != NULL);
    if (game->state != GAME_STATE_GAMEPLAY) return;

    if (IsKeyPressed(KEY_SPACE)) 
    {
        Actor* a = ACTOR_Create(game);
        if (a) 
        {
            float x = (float)GetRandomValue(-8, 7);
            float z = (float)GetRandomValue(-8, 7);
            ACTOR_SetPosition(a, (Vector3) { x, 0.5f, z });
            ACTOR_SetScale(a, 0.6f);
            Color colors[] = { RED, ORANGE, YELLOW, GREEN, SKYBLUE, PURPLE };
            MeshComponent* mc = MESH_COMPONENT_Create(a, &SMeshes[L3_MESH_CUBE], &SMaterial);
			mc->tint = colors[GetRandomValue(0, 5)];
            SPIN_COMPONENT_Create(a, 1.0f + (float)GetRandomValue(0, 30) / 10.0f);
        }
    }

    if (IsKeyPressed(KEY_S)) 
    {
        Actor* a = ACTOR_Create(game);
        if (a) 
        {
            float x = (float)GetRandomValue(-8, 7);
            float z = (float)GetRandomValue(-8, 7);
            ACTOR_SetPosition(a, (Vector3) { x, 0.8f, z });
            MeshComponent* mc = MESH_COMPONENT_Create(a, &SMeshes[L3_MESH_SPHERE], &SMaterial);
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
        GAME_ChangeLevel(game, &LEVEL_4);
    }
}

static void LEVEL_3_Render3D(Game* game) 
{
	assert(game != NULL);
    DrawGrid(20, 1.0f);
    for (int i = 0; i < game->actorCount; i++)
    {
        Actor* a = game->actors[i];
        if (a->state != ACTOR_STATE_ACTIVE) continue;
        if (!ACTOR_GetComponentOfType(a, COMPONENT_MESH)) continue;

        Vector3 fwd = ACTOR_GetForward(a);
        DrawLine3D(ACTOR_GetWorldPosition(a),
            Vector3Add(ACTOR_GetWorldPosition(a), Vector3Scale(fwd, 1.5f)),
            YELLOW);
    }
}

static int LEVEL_3_RenderHUD(Game* game, int y) 
{
	assert(game != NULL);
    const int step = 22;
    (void)game;

    DrawText("Level 3 - MeshComponent", 10, y, 18, WHITE);
    y += step + 4;

    DrawText("SPACE - Spawn cube   S - Spawn sphere   V - Toggle vis", 10, y, 16, LIGHTGRAY);
    y += step;
    DrawText("BACKSPACE - Kill last", 10, y, 16, LIGHTGRAY);
    y += step;
    DrawText("TAB - Next level", 10, y, 16, YELLOW);
    y += step;

    return y;
}

Level LEVEL_3 = 
{
    .name = "3 - MeshComponent",
    .Init = LEVEL_3_Init,
    .Shutdown = LEVEL_3_Shutdown,
    .ProcessInput = LEVEL_3_Input,
    .Render3D = LEVEL_3_Render3D,
    .RenderHUD = LEVEL_3_RenderHUD,
};
