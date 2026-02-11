#include "demo.h"
#include "game.h"
#include "actor.h"
#include "component.h"
#include "rlgl.h"
#include <assert.h>
#include <stdlib.h>
#include <math.h>

typedef struct 
{
    Component base;
    float     angular_speed;
} SpinComponent;

static void spin_comp_update(Component *self, float dt) 
{
    SpinComponent *spin = (SpinComponent *)self;
    Actor *owner = self->owner;

    Quaternion inc = QuaternionFromAxisAngle(
        (Vector3){ 0.0f, 1.0f, 0.0f }, spin->angular_speed * dt);
    ACTOR_SetRotation(owner, QuaternionMultiply(owner->rotation, inc));
}

static SpinComponent *SpinComponent_Create(Actor *owner, float speed) 
{
    SpinComponent *self = malloc(sizeof(SpinComponent));
    if (!self) return NULL;
    COMPONENT_Init(&self->base, owner, COMPONENT_SPIN, 100);
    self->base.onUpdate = spin_comp_update;
    self->angular_speed  = speed;
    return self;
}

typedef struct 
{
    Component base;
    float     bob_time;
    float     base_y;
    float     amplitude;
    float     frequency;
} BobComponent;

static void bob_comp_update(Component *self, float dt) 
{
    BobComponent *bob = (BobComponent *)self;
    bob->bob_time += dt;

    Vector3 pos = self->owner->position;
    pos.y = bob->base_y + sinf(bob->bob_time * bob->frequency) * bob->amplitude;
    ACTOR_SetPosition(self->owner, pos);
}

static BobComponent *BobComponent_Create(Actor *owner, float base_y,
                                          float amplitude, float frequency)
{
    BobComponent *self = malloc(sizeof(BobComponent));
    if (!self) return NULL;
    COMPONENT_Init(&self->base, owner, COMPONENT_NONE, 50);
    self->base.onUpdate = bob_comp_update;
    self->bob_time  = 0.0f;
    self->base_y    = base_y;
    self->amplitude = amplitude;
    self->frequency = frequency;
    return self;
}

void DEMO_Init(Game *game) 
{
    SetRandomSeed(GetTime());

    /* Static cubes along X (no components) */
    for (int i = 0; i < 5; i++) 
    {
        Actor *a = malloc(sizeof(Actor));
        ACTOR_Init(a, game);
        ACTOR_SetPosition(a, (Vector3){ (float)(i * 3 - 6), 0.5f, 0.0f });
    }

    /* Spinner (SpinComponent only) */
    {
        Actor *a = malloc(sizeof(Actor));
        ACTOR_Init(a, game);
        ACTOR_SetPosition(a, (Vector3){ 0.0f, 0.5f, 5.0f });
        ACTOR_SetScale(a, 1.3f);
        SpinComponent_Create(a, 1.5f);
    }

    /* Bobber (BobComponent only) */
    {
        Actor *a = malloc(sizeof(Actor));
        ACTOR_Init(a, game);
        ACTOR_SetPosition(a, (Vector3){ 0.0f, 1.0f, -5.0f });
        BobComponent_Create(a, 1.0f, 0.6f, 2.5f);
    }

    /* Spinner + Bobber (composition) */
    {
        Actor *a = malloc(sizeof(Actor));
        ACTOR_Init(a, game);
        ACTOR_SetPosition(a, (Vector3){ 5.0f, 1.0f, 0.0f });
        ACTOR_SetScale(a, 1.2f);
        SpinComponent_Create(a, 2.0f);
        BobComponent_Create(a, 1.0f, 0.8f, 2.0f);
    }
}

void DEMO_Shutdown(Game *game) 
{
    /* Nothing demo-specific to clean up.
     * Engine destroys all actors automatically. */
    (void)game;
}

void DEMO_ProcessInput(Game *game) 
{
    assert (game != NULL);
    if (game->state != GAME_STATE_GAMEPLAY) return;

    /* SPACE: spawn spinning actor */
    if (IsKeyPressed(KEY_SPACE)) 
    {
        Actor *a = malloc(sizeof(Actor));
        if (a) 
        {
            ACTOR_Init(a, game);
            float x = (float)(GetRandomValue(-8, 8));
            float z = (float)(GetRandomValue(-8, 8));
            ACTOR_SetPosition(a, (Vector3){ x, 0.5f, z });
            ACTOR_SetScale(a, 0.6f);
            SpinComponent_Create(a, 1.5f + (float)(GetRandomValue(0, 30)) / 10.0f);
        }
    }

    /* B: spawn spinning + bobbing actor */
    if (IsKeyPressed(KEY_B)) 
    {
        Actor *a = malloc(sizeof(Actor));
        if (a) {
            ACTOR_Init(a, game);
            float x = (float)(GetRandomValue(-8, 8));
            float z = (float)(GetRandomValue(-8, 8));
            ACTOR_SetPosition(a, (Vector3){ x, 1.0f, z });
            SpinComponent_Create(a, 2.0f);
            BobComponent_Create(a, 1.0f, 0.5f, 3.0f);
        }
    }

    /* BACKSPACE: kill last actor */
    if (IsKeyPressed(KEY_BACKSPACE)) {
        if (game->actorCount > 0) {
            game->actors[game->actorCount - 1]->state = ACTOR_STATE_DEAD;
        }
    }
}

static Color actor_debug_color(Actor *actor) {
    bool has_spin = (ACTOR_GetComponentOfType(actor, COMPONENT_SPIN) != NULL);
    if (has_spin && actor->componentCount >= 2) return GREEN;
    if (has_spin)                                 return RED;
    if (actor->componentCount > 0)               return SKYBLUE;
    return GRAY;
}

void DEMO_Render3D(Game *game) {
    for (int i = 0; i < game->actorCount; i++) 
    {
        Actor *actor = game->actors[i];
        if (actor->state != ACTOR_STATE_ACTIVE) continue;

        Color color = actor_debug_color(actor);

        rlPushMatrix();
            rlMultMatrixf(MatrixToFloatV(actor->worldTransform).v);
            DrawCube((Vector3){ 0 }, 1.0f, 1.0f, 1.0f, color);
            DrawCubeWires((Vector3){ 0 }, 1.0f, 1.0f, 1.0f, BLACK);
        rlPopMatrix();

        /* Forward direction indicator */
        Vector3 fwd = ACTOR_GetForward(actor);
        Vector3 start = actor->position;
        Vector3 end = Vector3Add(start, Vector3Scale(fwd, 1.5f));
        DrawLine3D(start, end, YELLOW);
    }
}

int DEMO_RenderHUD(Game *game, int y) 
{
    const int step = 22;
    (void)game;

    /* Color legend */
    DrawText("Actor types:", 10, y, 18, WHITE);
    y += step;
    DrawRectangle(10, y + 2, 14, 14, GRAY);
    DrawText("  Static (no components)", 28, y, 16, LIGHTGRAY);
    y += step;
    DrawRectangle(10, y + 2, 14, 14, RED);
    DrawText("  SpinComponent only", 28, y, 16, LIGHTGRAY);
    y += step;
    DrawRectangle(10, y + 2, 14, 14, SKYBLUE);
    DrawText("  BobComponent only", 28, y, 16, LIGHTGRAY);
    y += step;
    DrawRectangle(10, y + 2, 14, 14, GREEN);
    DrawText("  Spin + Bob (composition)", 28, y, 16, LIGHTGRAY);
    y += step * 2;

    DrawText("Controls:", 10, y, 18, WHITE);
    y += step;
    DrawText("  SPACE     - Spawn spinning actor", 10, y, 16, LIGHTGRAY);
    y += step;
    DrawText("  B         - Spawn spinning + bobbing", 10, y, 16, LIGHTGRAY);
    y += step;
    DrawText("  BACKSPACE - Kill last actor", 10, y, 16, LIGHTGRAY);
    y += step;

    return y;
}