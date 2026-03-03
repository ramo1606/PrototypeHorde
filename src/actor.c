/*******************************************************************************************
*
*   actor.c — Actor Implementation
*
********************************************************************************************/
#include "actor.h"
#include "component.h"
#include "game.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

static void ACTOR_Init(Actor* actor, Game* game);

/*------------------------------------------------------------------------------------
 * ACTOR_Create
 * 
 *   Factory function — allocates an Actor from the object pool (MEMORY_AllocActor)
 *   and initializes all fields to safe defaults. The actor is automatically registered
 *   with the Game via ACTOR_Init → GAME_AddActor.
 * 
 *   Uses an Object Pool (ObjPool) instead of malloc for cache-friendly allocation
 *   and O(1) alloc/free without fragmentation. See memory.h for pool details.
 *------------------------------------------------------------------------------------*/
Actor* ACTOR_Create(Game* game)
{
    assert(game != NULL);

    Actor* actor = MEMORY_AllocActor(&game->memory);
    if (!actor) return NULL;

    ACTOR_Init(actor, game);
    return actor;
}

/*------------------------------------------------------------------------------------
 * ACTOR_Init (static)
 * 
 *   Internal initialization — sets all fields to safe defaults:
 *   - Initializes the root SceneComponent
 *   - State = ACTIVE, Type = NONE, Tags = 0
 *   - All virtual function pointers = NULL
 *   - Component array zeroed out
 *   - Registers with the Game (goes to pending list if mid-update)
 *------------------------------------------------------------------------------------*/
void ACTOR_Init(Actor* actor, Game* game) 
{
	assert(actor != NULL);
	assert(game != NULL);

    /* The root SceneComponent is embedded (not heap-allocated), so we use
       InitRoot instead of the normal Init path which would try to register 
       it as a regular component with ACTOR_AddComponent. */
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

/*------------------------------------------------------------------------------------
 * ACTOR_Destroy
 * 
 *   Teardown sequence:
 *   1. Destroy all components in reverse order (last-to-first). Each COMPONENT_Destroy
 *      call decrements componentCount, so we always pop from the back.
 *   2. Call the actor's custom Destroy callback (if set) for gameplay cleanup.
 *   3. Unregister from the Game's actor list.
 *   4. Free the Actor back to the object pool.
 * 
 *   Components are destroyed before the actor callback so that the callback
 *   can still reference the actor's fields (game, type, etc.) but not components.
 *------------------------------------------------------------------------------------*/
void ACTOR_Destroy(Actor* actor) 
{
	assert(actor != NULL);

    /* Step 1: Destroy all components (reverse order) */
    while (actor->componentCount > 0) 
    {
        int lastIndex = actor->componentCount - 1;
		Component* comp = actor->components[lastIndex];
        COMPONENT_Destroy(comp);
    }

    /* Step 2: Custom actor cleanup */
    if (actor->Destroy)
    {
        actor->Destroy(actor);
    }

    /* Step 3 & 4: Unregister and free to pool */
	Game* game = actor->game;
	GAME_RemoveActor(game, actor);
    MEMORY_FreeActor(&game->memory, actor);
}

/*------------------------------------------------------------------------------------
 * ACTOR_Update
 * 
 *   Per-tick update sequence (called at fixed timestep, typically 60Hz):
 *   1. Skip if not ACTIVE (paused or dead actors don't update).
 *   2. Recompute world transform (only if dirty — see SceneComponent).
 *   3. Update all components in updateOrder.
 *   4. Call the actor's custom Update callback for gameplay logic.
 * 
 *   Transform is computed BEFORE components so that components see the latest
 *   world-space position/rotation when they run their logic.
 *------------------------------------------------------------------------------------*/
void ACTOR_Update(Actor* actor, float deltaTime) 
{
	assert(actor != NULL);

    if (actor->state != ACTOR_STATE_ACTIVE) return;

    /* Step 1: Ensure world transform is up to date */
    ACTOR_ComputeWorldTransform(actor);
    /* Step 2: Tick all components */
	ACTOR_UpdateComponents(actor, deltaTime);
    
    /* Step 3: Actor-level gameplay logic */
    if (actor->Update)
    {
        actor->Update(actor, deltaTime);
    }
}

/*------------------------------------------------------------------------------------
 * ACTOR_UpdateComponents
 * 
 *   Iterates the component array and calls each component's Update callback.
 *   Components are stored sorted by updateOrder (set during COMPONENT_Init),
 *   so dependencies naturally resolve (e.g., MoveComponent runs before colliders).
 *------------------------------------------------------------------------------------*/
void ACTOR_UpdateComponents(Actor* actor, float deltaTime)
{
	assert(actor != NULL);

    for (int i = 0; i < actor->componentCount; i++)
    {
        if (actor->components[i]->Update)
        {
            actor->components[i]->Update(actor->components[i], deltaTime);
        }
    }
}

/*------------------------------------------------------------------------------------
 * ACTOR_ProcessInput
 * 
 *   Per-frame input processing:
 *   1. Skip if not ACTIVE.
 *   2. Call Input on all components (e.g., a PlayerInputComponent could poll keys).
 *   3. Call the actor's own Input callback.
 * 
 *   This runs every frame (not at fixed timestep) so input feels responsive.
 *------------------------------------------------------------------------------------*/
void ACTOR_ProcessInput(Actor* actor) 
{
	assert(actor != NULL);

    if (actor->state != ACTOR_STATE_ACTIVE) return;

    /* Component-level input */
    for (int i = 0; i < actor->componentCount; i++) 
    {
        if (actor->components[i]->Input) 
        {
            actor->components[i]->Input(actor->components[i]);
        }
    }

    /* Actor-level input */
    if (actor->Input) 
    {
        actor->Input(actor);
    }
}

/*------------------------------------------------------------------------------------
 * ACTOR_ComputeWorldTransform
 * 
 *   Delegates to the root SceneComponent's transform computation.
 *   Uses a dirty flag to avoid redundant recalculations — only recomputes
 *   if position, rotation, or scale changed since last computation.
 *------------------------------------------------------------------------------------*/
void ACTOR_ComputeWorldTransform(Actor* actor) 
{
	assert(actor != NULL);
    SCENE_COMPONENT_ComputeWorldTransform(&actor->root);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Transform Getters
 *  
 *  These extract direction vectors from the world transform matrix.
 *  See scene_component.c for the matrix layout explanation.
 * ═══════════════════════════════════════════════════════════════════════════ */

/*------------------------------------------------------------------------------------
 * ACTOR_GetForward
 * 
 *   Returns the actor's forward direction in world space.
 *   In our coordinate system forward is -Z (OpenGL convention), so we
 *   negate the third column of the world transform matrix.
 * 
 *   TODO: Verify coordinate system — could be Z+ depending on asset pipeline.
 *------------------------------------------------------------------------------------*/
Vector3 ACTOR_GetForward(Actor* actor) 
{
	assert(actor != NULL);
    return SCENE_COMPONENT_GetForward(&actor->root);
}

/* Get actor's right direction in world space (first column of world matrix). */
Vector3 ACTOR_GetRight(Actor* actor) 
{
	assert(actor != NULL);
    return SCENE_COMPONENT_GetRight(&actor->root);
}

/* Get actor's up direction in world space (second column of world matrix). */
Vector3 ACTOR_GetUp(Actor* actor)
{
	assert(actor != NULL);
    return SCENE_COMPONENT_GetUp(&actor->root);
}

/* Get actor's position in world space (translation column of world matrix). */
Vector3 ACTOR_GetWorldPosition(Actor* actor)
{
    assert(actor != NULL);
    return SCENE_COMPONENT_GetWorldPosition(&actor->root);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Transform Setters
 * 
 *  Each setter modifies the local transform values and marks the
 *  SceneComponent as dirty so the world transform is recomputed on next use.
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Set local position and invalidate cached world transform. */
void ACTOR_SetPosition(Actor* actor, Vector3 pos) 
{
	assert(actor != NULL);

    actor->root.position = pos;
    SCENE_COMPONENT_MarkDirty(&actor->root);
}

/* Set local rotation (euler angles in radians, XYZ order) and invalidate. */
void ACTOR_SetRotation(Actor* actor, Vector3 euler) 
{
    assert(actor != NULL);
    actor->root.rotation = euler;
    SCENE_COMPONENT_MarkDirty(&actor->root);
}

/* Set uniform scale (all axes equal) and invalidate. Scale must be > 0. */
void ACTOR_SetScale(Actor* actor, float scale) 
{
	assert(scale > 0.0f);
	assert(actor != NULL);

    actor->root.scale = (Vector3){ scale, scale, scale };
    SCENE_COMPONENT_MarkDirty(&actor->root);
}

/*------------------------------------------------------------------------------------
 * ACTOR_RotateToNewForward
 * 
 *   Rotates the actor so its forward vector aligns with the given direction.
 *   Only rotates around the Y axis (yaw) — ignores vertical component.
 * 
 *   Algorithm:
 *   1. Project the desired forward onto the XZ plane (flatten Y to 0).
 *   2. Normalize the flattened vector.
 *   3. Compute yaw angle using atan2(-x, -z).
 *      The negation is because our forward is -Z, so we convert to the
 *      standard atan2 frame where angle 0 = forward = -Z.
 *   4. Set rotation to (0, yaw, 0).
 * 
 *   Note: This snaps instantly. For smooth rotation, use a lerp approach
 *   in a CharacterMovementComponent (planned for future phases).
 *------------------------------------------------------------------------------------*/
void ACTOR_RotateToNewForward(Actor* actor, Vector3 forward) 
{
    assert(actor != NULL);

    /* Step 1: Flatten to XZ plane */
    Vector3 flatFwd = { forward.x, 0.0f, forward.z };
    float len = Vector3Length(flatFwd);
    if (len < 0.0001f) return;

    /* Step 2: Normalize */
    flatFwd = Vector3Scale(flatFwd, 1.0f / len);

    /* Step 3: Compute yaw — atan2(-x, -z) maps our -Z forward to angle 0 */
    float yaw = atan2f(-flatFwd.x, -flatFwd.z);
    ACTOR_SetRotation(actor, (Vector3) { 0.0f, yaw, 0.0f });
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Component Management
 * 
 *  Components are stored in a flat array sorted by updateOrder.
 *  This ensures deterministic update ordering without runtime sorting.
 * ═══════════════════════════════════════════════════════════════════════════ */

/*------------------------------------------------------------------------------------
 * ACTOR_AddComponent
 * 
 *   Inserts a component into the actor's array maintaining updateOrder sort.
 * 
 *   Algorithm: Insertion Sort (single element)
 *   1. Find the first component with a higher updateOrder → that's the insert index.
 *   2. Shift all elements from insertIdx..end one position right.
 *   3. Place the new component at insertIdx.
 * 
 *   Time complexity: O(n) where n = componentCount. Fine for small arrays (max 16).
 *------------------------------------------------------------------------------------*/
void ACTOR_AddComponent(Actor* actor, Component* comp) 
{
    assert(actor != NULL);
    assert(comp != NULL);

    if (actor->componentCount >= ACTOR_MAX_COMPONENTS) 
    {
        TraceLog(LOG_WARNING, "ACTOR: Component list full (%d)", ACTOR_MAX_COMPONENTS);
        return;
    }

    /* Find insertion point to maintain sort by updateOrder */
    int insertIdx = actor->componentCount;
    for (int i = 0; i < actor->componentCount; i++) 
    {
        if (comp->updateOrder < actor->components[i]->updateOrder) 
        {
            insertIdx = i;
            break;
        }
    }

    /* Shift elements right to make room */
    for (int i = actor->componentCount; i > insertIdx; i--) 
    {
        actor->components[i] = actor->components[i - 1];
    }

    actor->components[insertIdx] = comp;
    actor->componentCount++;
}

/*------------------------------------------------------------------------------------
 * ACTOR_RemoveComponent
 * 
 *   Removes a component from the actor's array by linear search, then shifts
 *   remaining elements left to fill the gap (preserves order).
 * 
 *   Time complexity: O(n) for search + O(n) for shift.
 *------------------------------------------------------------------------------------*/
void ACTOR_RemoveComponent(Actor* actor, Component* comp) 
{
    assert(actor != NULL);
    assert(comp != NULL);

    for (int i = 0; i < actor->componentCount; i++) 
    {
        if (actor->components[i] == comp) 
        {
            /* Shift left to fill gap (maintains order) */
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

/*------------------------------------------------------------------------------------
 * ACTOR_GetComponentOfType
 * 
 *   Linear search for the first component matching the given type.
 *   Returns NULL if no component of that type is attached.
 *------------------------------------------------------------------------------------*/
Component *ACTOR_GetComponentOfType(Actor* actor, ComponentType type) 
{
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

/*------------------------------------------------------------------------------------
 * ACTOR_GetComponentsOfType
 * 
 *   Fills outArray with all components matching the given type, up to maxResults.
 *   Returns the number of components found.
 *------------------------------------------------------------------------------------*/
int ACTOR_GetComponentsOfType(Actor* actor, ComponentType type, Component** outArray, int maxResults)
{
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

/*------------------------------------------------------------------------------------
 * ACTOR_HasTag
 * 
 *   Checks if a specific tag bit is set in the actor's tag bitmask.
 *   Tags use bitwise operations so an actor can have multiple tags simultaneously.
 *   Example: actor->tags = TAG_PLAYER | TAG_ALIVE;
 *            ACTOR_HasTag(actor, TAG_PLAYER) → true
 *------------------------------------------------------------------------------------*/
bool ACTOR_HasTag(Actor* actor, unsigned int tag)
{
    assert(actor != NULL);
    return (actor->tags & tag) != 0;
}