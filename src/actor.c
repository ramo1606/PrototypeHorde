#include "actor.h"
#include "component.h"
#include "game.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

static void ACTOR_Init(Actor* actor, Game* game);

Actor* ACTOR_Create(Game* game)
{
    /*
     * Allocate a slot from the ObjPool, initialise it, and register it
     * with the game.  Returns NULL if the pool is exhausted.
     */
    assert(game != NULL);

    Actor* actor = MEMORY_AllocActor(&game->memory);
    if (!actor) return NULL;

    ACTOR_Init(actor, game);
    return actor;
}

void ACTOR_Init(Actor* actor, Game* game) 
{
    /*
     * Set all fields to their default/inactive values and call
     * GAME_AddActor which either adds directly to the live list or to the
     * pending queue depending on whether a fixed update is in progress.
     */
	assert(actor != NULL);
	assert(game != NULL);

    SCENE_COMPONENT_InitRoot(&actor->root, actor);

    actor->state = ACTOR_STATE_ACTIVE;
    actor->type  = ACTOR_TYPE_NONE;
    actor->tags = 0;
    actor->game  = game;

    actor->Update = NULL;
    actor->Input = NULL;
    actor->Destroy = NULL;

    actor->componentCount = 0;
    memset(actor->components, 0, sizeof(actor->components));

	GAME_AddActor(game, actor);
}

void ACTOR_Destroy(Actor* actor) 
{
    /*
     * Teardown order:
     *   1. Destroy all attached components (back-to-front so removal
     *      does not shift indices we have not visited yet).
     *   2. Invoke the actor's own Destroy callback (custom cleanup).
     *   3. Unregister from the game's actor list.
     *   4. Return the memory slot to the ObjPool.
     */
	assert(actor != NULL);

    while (actor->componentCount > 0) 
    {
        int lastIndex = actor->componentCount - 1;
		Component* comp = actor->components[lastIndex];
        COMPONENT_Destroy(comp);
    }

    if (actor->Destroy)
    {
        actor->Destroy(actor);
    }

	Game* game = actor->game;
	GAME_RemoveActor(game, actor);
    MEMORY_FreeActor(&game->memory, actor);
}

void ACTOR_Update(Actor* actor, float deltaTime) 
{
    /*
     * Per-fixed-tick update:
     *   1. Recompute world transform (picks up any SetPosition/SetRotation
     *      calls from the previous tick or from Init).
     *   2. Update all attached components in sorted order.
     *   3. Invoke the actor's own Update callback if set.
     */
	assert(actor != NULL);

    if (actor->state != ACTOR_STATE_ACTIVE) return;

    ACTOR_ComputeWorldTransform(actor);
	ACTOR_UpdateComponents(actor, deltaTime);
    
    if (actor->Update)
    {
        actor->Update(actor, deltaTime);
    }
}

void ACTOR_UpdateComponents(Actor* actor, float deltaTime)
{
    /*
     * Iterate the sorted component array and call each component's Update
     * callback.  Components without an Update callback are skipped.
     * The array is pre-sorted by updateOrder so execution order is stable.
     */
	assert(actor != NULL);

    for (int i = 0; i < actor->componentCount; i++)
    {
        if (actor->components[i]->Update)
        {
            actor->components[i]->Update(actor->components[i], deltaTime);
        }
    }
}

void ACTOR_ProcessInput(Actor* actor) 
{
    /*
     * Forward input to components first (in sorted order), then to the
     * actor's own Input callback.  This mirrors the update order so input
     * and update are always processed in the same priority sequence.
     */
	assert(actor != NULL);

    if (actor->state != ACTOR_STATE_ACTIVE) return;

    for (int i = 0; i < actor->componentCount; i++) 
    {
        if (actor->components[i]->Input) 
        {
            actor->components[i]->Input(actor->components[i]);
        }
    }

    if (actor->Input) 
    {
        actor->Input(actor);
    }
}

void ACTOR_ComputeWorldTransform(Actor* actor) 
{
    /*
     * Delegate to the root SceneComponent which propagates down the scene
     * graph to all child nodes.
     */
	assert(actor != NULL);
    SCENE_COMPONENT_ComputeWorldTransform(&actor->root);
}

//TODO: revisar que el sistema de coordenadas sea correcto, podría ser que el forward sea Z+ y no X+
Vector3 ACTOR_GetForward(Actor* actor) 
{
	assert(actor != NULL);
    return SCENE_COMPONENT_GetForward(&actor->root);
}

Vector3 ACTOR_GetRight(Actor* actor) 
{
	assert(actor != NULL);
    return SCENE_COMPONENT_GetRight(&actor->root);
}

Vector3 ACTOR_GetUp(Actor* actor)
{
	assert(actor != NULL);
    return SCENE_COMPONENT_GetUp(&actor->root);
}

Vector3 ACTOR_GetWorldPosition(Actor* actor)
{
    assert(actor != NULL);
    return SCENE_COMPONENT_GetWorldPosition(&actor->root);
}

void ACTOR_SetPosition(Actor* actor, Vector3 pos) 
{
	assert(actor != NULL);

    actor->root.position = pos;
    SCENE_COMPONENT_MarkDirty(&actor->root);
}

void ACTOR_SetRotation(Actor* actor, Vector3 euler) 
{
    assert(actor != NULL);
    actor->root.rotation = euler;
    SCENE_COMPONENT_MarkDirty(&actor->root);
}

void ACTOR_SetScale(Actor* actor, float scale) 
{
	assert(scale > 0.0f);
	assert(actor != NULL);

    actor->root.scale = (Vector3){ scale, scale, scale };
    SCENE_COMPONENT_MarkDirty(&actor->root);
}

void ACTOR_RotateToNewForward(Actor* actor, Vector3 forward) 
{
    /*
     * Project the desired forward direction onto the XZ plane to compute
     * a yaw-only rotation.  Pitch and roll are ignored so the actor never
     * tilts.  atan2(-x, -z) maps the OpenGL -Z forward convention to a
     * Y-rotation angle.
     */
    assert(actor != NULL);

    Vector3 flatFwd = { forward.x, 0.0f, forward.z };
    float len = Vector3Length(flatFwd);
    if (len < 0.0001f) return;

    flatFwd = Vector3Scale(flatFwd, 1.0f / len);

    float yaw = atan2f(-flatFwd.x, -flatFwd.z);
    ACTOR_SetRotation(actor, (Vector3) { 0.0f, yaw, 0.0f });
}

void ACTOR_AddComponent(Actor* actor, Component* comp) 
{
    /*
     * Insertion sort by updateOrder (ascending).  Scan for the first
     * existing component with a higher order, shift everything right to
     * make room, and insert.  O(n) worst case but n <= ACTOR_MAX_COMPONENTS
     * so this is negligible.
     */
    assert(actor != NULL);
    assert(comp != NULL);

    if (actor->componentCount >= ACTOR_MAX_COMPONENTS) 
    {
        TraceLog(LOG_WARNING, "ACTOR: Component list full (%d)", ACTOR_MAX_COMPONENTS);
        return;
    }

    int insertIdx = actor->componentCount;
    for (int i = 0; i < actor->componentCount; i++) 
    {
        if (comp->updateOrder < actor->components[i]->updateOrder) 
        {
            insertIdx = i;
            break;
        }
    }

    for (int i = actor->componentCount; i > insertIdx; i--) 
    {
        actor->components[i] = actor->components[i - 1];
    }

    actor->components[insertIdx] = comp;
    actor->componentCount++;
}

void ACTOR_RemoveComponent(Actor* actor, Component* comp) 
{
    /*
     * Linear scan, shift-left removal.  Maintains sorted order without
     * a re-sort.  O(n) but component removal is infrequent.
     */
    assert(actor != NULL);
    assert(comp != NULL);

    for (int i = 0; i < actor->componentCount; i++) 
    {
        if (actor->components[i] == comp) 
        {
            for (int j = i; j < actor->componentCount - 1; j++) 
            {
                actor->components[j] = actor->components[j + 1];
            }
            actor->components[actor->componentCount - 1] = NULL;
            actor->componentCount--;
            return;
        }
    }
}

Component *ACTOR_GetComponentOfType(Actor* actor, ComponentType type) 
{
    /*
     * Linear scan — returns the first match.  For most actor archetypes
     * there is at most one component of each type, so this is O(1) in
     * practice.
     */
    assert(actor != NULL);

    for (int i = 0; i < actor->componentCount; i++) 
    {
        if (actor->components[i]->type == type) 
        {
            return actor->components[i];
        }
    }
    return NULL;
}

int ACTOR_GetComponentsOfType(Actor* actor, ComponentType type, Component** outArray, int maxResults)
{
    /*
     * Collect all components matching type into outArray (up to maxResults).
     * Useful when multiple components of the same type are expected (e.g.
     * multiple BoxComponents for a compound collider).
     */
    assert(actor != NULL);
    assert(outArray != NULL);
    assert(maxResults > 0);

    int count = 0;
    for (int i = 0; i < actor->componentCount && count < maxResults; i++)
    {
        if (actor->components[i]->type == type)
        {
            outArray[count++] = actor->components[i];
        }
    }
    return count;
}

bool ACTOR_HasTag(Actor* actor, unsigned int tag)
{
    /*
     * Bitwise AND test — true if any bit of tag is set in the actor's
     * tag bitmask.  Designed for fast tag-based scene queries.
     */
    assert(actor != NULL);
    return (actor->tags & tag) != 0;
}