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
    if (type >= 0 && type < NUM_COMPONENT_TYPES)
        return ComponentTypeNames[type];
    return "Unknown";
}
