#include "component.h"
#include "actor.h"
#include "memory.h"
#include "game.h"
#include <stdlib.h>
#include <assert.h>

static const char* ComponentTypeNames[NUM_COMPONENT_TYPES] =
{
    "None",
    "Scene",
    "Mesh",
    "Move",
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

void COMPONENT_Init(Component* comp, Actor* owner, ComponentType type, int updateOrder) 
{
    /*
     * Zero-fill the common fields, then register the component with the
     * owner actor's sorted component list.  ACTOR_AddComponent uses
     * insertion sort on updateOrder so lower-order components always run
     * before higher-order ones.
     */
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

void COMPONENT_Destroy(Component* comp) 
{
    /*
     * Teardown order:
     *   1. Invoke the optional Destroy callback (e.g. MeshComponent
     *      unregisters from the Renderer here).
     *   2. Unregister from the owner's component array.
     *   3. Return memory to the component MemPool.
     */
	assert(comp != NULL);

    if (comp->Destroy) 
    {
        comp->Destroy(comp);
    }

	ACTOR_RemoveComponent(comp->owner, comp);

	MEMORY_FreeComponent(&(comp->owner->game->memory), comp);
}

const char* COMPONENT_GetTypeName(ComponentType type)
{
    /*
     * Simple bounds-checked lookup into the static name table.
     * Returns "Unknown" for any value outside the valid range so callers
     * can safely use the result in log messages without a crash.
     */
    if (type >= 0 && type < NUM_COMPONENT_TYPES)
        return ComponentTypeNames[type];
    return "Unknown";
}
