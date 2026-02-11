#include "component.h"
#include "actor.h"
#include <stdlib.h>

void COMPONENT_Init(Component *comp, Actor *owner, ComponentType type, int updateOrder) 
{
    comp->owner        = owner;
    comp->type         = type;
    comp->updateOrder = updateOrder;

    comp->onUpdate          = NULL;
    comp->onInput           = NULL;
    comp->onWorldTransform = NULL;
    comp->onDestroy         = NULL;

    ACTOR_AddComponent(owner, comp);
}

void COMPONENT_Destroy(Component *comp) 
{
    if (!comp) return;

    if (comp->onDestroy) 
    {
        comp->onDestroy(comp);
    }

    ACTOR_RemoveComponent(comp->owner, comp);
    free(comp);
}