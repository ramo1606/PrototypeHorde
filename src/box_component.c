#include "box_component.h"
#include "collision.h"
#include "actor.h"
#include "game.h"
#include "physics_world.h"
#include "memory.h"
#include <assert.h>

static void BoxComponentUpdate(Component* self, float deltaTime)
{
    /*
     * Recompute the world-space AABB each fixed tick.
     *
     * The actor's world transform is recomputed first (in case it is
     * still dirty from a SetPosition call), then the local objectBox is
     * transformed to world space using the 8-corner AABB method
     * (COLLISION_TransformAABB).  The result is stored in worldBox for
     * use in raycasts, overlap queries, and pairwise collision tests.
     */
    (void)deltaTime;
    BoxComponent* bc = (BoxComponent*)self;
    Actor* owner = bc->base.owner;

    SCENE_COMPONENT_ComputeWorldTransform(&owner->root);

    bc->worldBox = COLLISION_TransformAABB(bc->objectBox, owner->root.worldTransform);
}

static void BoxComponentDestroy(Component* self)
{
    /*
     * Unregister from the physics world before memory is freed so the
     * world's box list never contains a dangling pointer.
     */
    BoxComponent* bc = (BoxComponent*)self;
    PHYS_WORLD_RemoveBox(&bc->base.owner->game->physWorld, bc);
}

BoxComponent* BOX_COMPONENT_Create(Actor* owner)
{
    /*
     * Allocate from the component pool, register at updateOrder 300
     * (after movement and camera so the world transform is final before
     * the AABB is computed), and register with the physics world.
     * Default layerMask = 0xFFFFFFFF collides with all layers.
     */
    assert(owner != NULL);

    BoxComponent* bc = (BoxComponent*)MEMORY_AllocComponent(
        &owner->game->memory, sizeof(BoxComponent));
    if (!bc) return NULL;

    COMPONENT_Init(&bc->base, owner, COMPONENT_TYPE_BOX, 300);
    bc->base.Update = BoxComponentUpdate;
    bc->base.Destroy = BoxComponentDestroy;

    bc->objectBox = (BoundingBox){ 0 };
    bc->worldBox = (BoundingBox){ 0 };
    bc->layerMask = 0xFFFFFFFF;     /* Default: collides with everything */
    bc->isTrigger = false;

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
    /*
     * Derive the local-space AABB directly from mesh vertex data via
     * Raylib's GetMeshBoundingBox utility.
     */
    assert(bc != NULL);
    bc->objectBox = GetMeshBoundingBox(mesh);
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
