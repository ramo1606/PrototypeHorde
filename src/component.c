#include "component.h"
#include "actor.h"
#include <stdlib.h>
#include <assert.h>

const char* ComponentTypeNames[NUM_COMPONENT_TYPES] =
{
    "Component",
    "MeshComponent",
    "MoveComponent"
};

void COMPONENT_Init(Component* comp, Actor* owner, ComponentType type, int updateOrder) 
{
	assert(comp != NULL);
	assert(owner != NULL);

    comp->owner        = owner;
    comp->type         = type;
    comp->updateOrder = updateOrder;

    comp->onUpdate          = NULL;
    comp->onInput           = NULL;
    comp->onWorldTransform = NULL;
    comp->onDestroy         = NULL;

	ACTOR_AddComponent(owner, comp);
}

void COMPONENT_Destroy(Component* comp) 
{
	assert(comp != NULL);

    if (comp->onDestroy) 
    {
        comp->onDestroy(comp);
    }
}