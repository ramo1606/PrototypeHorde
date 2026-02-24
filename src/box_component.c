#include "box_component.h"
#include "collision.h"
#include "actor.h"
#include "game.h"
#include "physics_world.h"
#include "memory.h"
#include <assert.h>

static void BoxComponentUpdate(Component* self, float deltaTime)
{
    (void)deltaTime;
    BoxComponent* bc = (BoxComponent*)self;
    Actor* owner = bc->component.owner;

    SCENE_COMPONENT_ComputeWorldTransform(&owner->root);

    bc->worldBox = COLLISION_TransformAABB(bc->objectBox, owner->root.worldTransform);
}

static void BoxComponentDestroy(Component* self)
{
    BoxComponent* bc = (BoxComponent*)self;
    PHYS_WORLD_RemoveBox(&bc->component.owner->game->physWorld, bc);
}

BoxComponent* BOX_COMPONENT_Create(Actor* owner)
{
    assert(owner != NULL);

    BoxComponent* bc = (BoxComponent*)MEMORY_AllocComponent(
        &owner->game->memory, sizeof(BoxComponent));
    if (!bc) return NULL;

    COMPONENT_Init(&bc->component, owner, COMPONENT_BOX, 300);
    bc->component.Update = BoxComponentUpdate;
    bc->component.Destroy = BoxComponentDestroy;

    bc->objectBox = (BoundingBox){ 0 };
    bc->worldBox = (BoundingBox){ 0 };

    PHYS_WORLD_AddBox(&owner->game->physWorld, bc);

    return bc;
}

void BOX_COMPONENT_SetObjectBox(BoxComponent* bc, BoundingBox objectBox)
{
    assert(bc != NULL);
    bc->objectBox = objectBox;
}

void BOX_COMPONENT_SetFromMesh(BoxComponent* bc, Mesh mesh)
{
    
}

BoundingBox BOX_COMPONENT_GetWorldBox(BoxComponent* bc)
{
    assert(bc != NULL);
    return bc->worldBox;
}

void BOX_COMPONENT_DrawWorldBox(BoxComponent* bc, Color color)
{
    assert(bc != NULL);
    DrawBoundingBox(bc->worldBox, color);
}
