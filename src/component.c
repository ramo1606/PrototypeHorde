#include "component.h"
#include "actor.h"
#include "memory.h"
#include "game.h"
#include <stdlib.h>
#include <assert.h>

static const char* ComponentTypeNames[NUM_COMPONENT_TYPES] =
{
    "None",             /* COMPONENT_NONE       = 0 */
    "Scene",            /* COMPONENT_SCENE      = 1 */
    "Mesh",             /* COMPONENT_MESH       = 2 */
    "Move",             /* COMPONENT_MOVE       = 3 */
    "Camera",           /* COMPONENT_CAMERA     = 4 */
    "CameraTPS",        /* COMPONENT_CAMERA_TPS = 5 */
    "Box",              /* COMPONENT_BOX        = 6 */
    "Sphere",           /* COMPONENT_SPHERE     = 7 */
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
