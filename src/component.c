/*******************************************************************************************
*
*   component.c — Base Component Implementation
*
********************************************************************************************/

#include "component.h"
#include "actor.h"
#include "memory.h"
#include "game.h"
#include <stdlib.h>
#include <assert.h>

/* ── Lookup table: ComponentType → human-readable string ─────────────────── */
/* Must stay in sync with the ComponentType enum order in component.h.        */
static const char* ComponentTypeNames[NUM_COMPONENT_TYPES] =
{
    "None",
    "Scene",
    "Mesh",
    "Move",
    "PlayerMovement",
    "Camera",
    "CameraTPS",
    "Box",
    "Sphere",
    "Capsule",
    "Model",
    "Audio Source",
    "AI Controller",
    "FSM"
};

/*------------------------------------------------------------------------------------
 * COMPONENT_Init
 *
 *   Initializes the base fields of a component and registers it with its owner Actor.
 *   All virtual function pointers are set to NULL — derived components override them
 *   after calling this function.
 *
 *   ACTOR_AddComponent inserts the component sorted by updateOrder (insertion sort),
 *   so components with lower updateOrder values tick first in the update loop.
 *   Typical update order values:
 *       10  = MoveComponent (movement first)
 *       200 = MeshComponent (visuals after logic)
 *       250 = CameraComponent (camera follows movement)
 *       300 = BoxComponent / SphereComponent (colliders after transforms settle)
 *------------------------------------------------------------------------------------*/
void COMPONENT_Init(Component* comp, Actor* owner, ComponentType type, int updateOrder) 
{
	assert(comp != NULL);
	assert(owner != NULL);

    comp->owner        = owner;
    comp->type         = type;
    comp->updateOrder = updateOrder;

    comp->Update          = NULL;
    comp->Input           = NULL;
    comp->Destroy         = NULL;

	ACTOR_AddComponent(owner, comp);
}

/*------------------------------------------------------------------------------------
 * COMPONENT_Destroy
 *
 *   Teardown sequence for any component:
 *   1. Call the custom Destroy callback (if set) — allows derived components to
 *      unregister from systems (e.g., remove mesh from Renderer, collider from PhysWorld).
 *   2. Remove from the owning Actor's component list.
 *   3. Free the memory back to the component pool (MemPool).
 *
 *   The order matters: the Destroy callback may need the owner reference,
 *   so we unregister from the Actor only after the callback completes.
 *------------------------------------------------------------------------------------*/
void COMPONENT_Destroy(Component* comp) 
{
	assert(comp != NULL);

    /* Step 1: Custom cleanup (unregister from subsystems) */
    if (comp->Destroy) 
    {
        comp->Destroy(comp);
    }

    /* Step 2: Remove from Actor's component array */
	ACTOR_RemoveComponent(comp->owner, comp);

    /* Step 3: Return memory to the pool */
	MEMORY_FreeComponent(&(comp->owner->game->memory), comp);
}

/*------------------------------------------------------------------------------------
 * COMPONENT_GetTypeName
 *
 *   Simple lookup into the static name table. Returns "Unknown" for out-of-range values.
 *   Useful for debug logging and editor tooling.
 *------------------------------------------------------------------------------------*/
const char* COMPONENT_GetTypeName(ComponentType type)
{
    if (type >= 0 && type < NUM_COMPONENT_TYPES)
        return ComponentTypeNames[type];
    return "Unknown";
}
