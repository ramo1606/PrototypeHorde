/*******************************************************************************************
*
*   box_component.c — Box Component Implementation
*
********************************************************************************************/
#include "box_component.h"
#include "collision.h"
#include "actor.h"
#include "game.h"
#include "physics_world.h"
#include "memory.h"
#include <assert.h>

/*------------------------------------------------------------------------------------
 * BoxComponentUpdate (static)
 *
 *   Per-tick update: transforms the local objectBox into world space.
 *   Uses COLLISION_TransformAABB which correctly handles rotation by testing
 *   all 8 corners of the box and computing the min/max extents.
 *
 *   This runs at updateOrder 300 (after movement and mesh updates).
 *------------------------------------------------------------------------------------*/
static void BoxComponentUpdate(Component* self, float deltaTime)
{
	assert(self != NULL);
    (void)deltaTime;

    if(self->type != COMPONENT_TYPE_BOX) return;
    BoxComponent* bc = (BoxComponent*)self;
    Actor* owner = bc->base.owner;

    /* Ensure the actor's world transform is up-to-date */
    SCENE_COMPONENT_ComputeWorldTransform(&owner->root);

    /* Transform local AABB to world space (handles rotation correctly) */
    bc->worldBox = COLLISION_TransformAABB(bc->objectBox, owner->root.worldTransform);
}

/*------------------------------------------------------------------------------------
 * BoxComponentDestroy (static)
 *
 *   Destroy callback — unregisters this box from the PhysWorld's collider list.
 *------------------------------------------------------------------------------------*/
static void BoxComponentDestroy(Component* self)
{
	assert(self != NULL);
	if (self->type != COMPONENT_TYPE_BOX) return;
    BoxComponent* bc = (BoxComponent*)self;
    PHYS_WORLD_RemoveBox(&bc->base.owner->game->physWorld, bc);
}

/*------------------------------------------------------------------------------------
 * BOX_COMPONENT_Create
 *
 *   Factory function — allocates from the component pool, initializes the base,
 *   sets up update and destroy callbacks, and registers with the PhysWorld.
 *
 *   Update order 300 ensures the box updates after the actor's position has been
 *   set by MoveComponent (order 10) and after the mesh (order 200).
 *------------------------------------------------------------------------------------*/
BoxComponent* BOX_COMPONENT_Create(Actor* owner)
{
    assert(owner != NULL);

    BoxComponent* bc = (BoxComponent*)MEMORY_AllocComponent(
        &owner->game->memory, sizeof(BoxComponent));
    if (!bc) return NULL;

    COMPONENT_Init(&bc->base, owner, COMPONENT_TYPE_BOX, 300);
    bc->base.Update = BoxComponentUpdate;
    bc->base.Destroy = BoxComponentDestroy;

    bc->objectBox = (BoundingBox){ 0 };
    bc->worldBox = (BoundingBox){ 0 };
    bc->layerMask = 0xFFFFFFFF;     /* Collide with all layers by default */
    bc->isTrigger = false;          /* Blocking collider by default       */

    /* Register with physics world for collision queries */
    PHYS_WORLD_AddBox(&owner->game->physWorld, bc);

    return bc;
}

/* Set the local-space AABB directly (e.g., from known dimensions). */
void BOX_COMPONENT_SetObjectBox(BoxComponent* bc, BoundingBox objectBox)
{
    assert(bc != NULL);
    bc->objectBox = objectBox;
}

/*------------------------------------------------------------------------------------
 * BOX_COMPONENT_SetFromMesh
 *
 *   Computes the local-space AABB from a Raylib Mesh's vertex data.
 *   Uses GetMeshBoundingBox which scans all vertices to find min/max extents.
 *   Convenient when you want the collider to match the visual geometry.
 *------------------------------------------------------------------------------------*/
void BOX_COMPONENT_SetFromMesh(BoxComponent* bc, Mesh mesh)
{
    assert(bc != NULL);
    bc->objectBox = GetMeshBoundingBox(mesh);
}

/* Get the cached world-space bounding box (updated each tick by BoxComponentUpdate). */
BoundingBox BOX_COMPONENT_GetWorldBox(BoxComponent* bc)
{
    assert(bc != NULL);
    return bc->worldBox;
}

/* Draw the world box as wireframe lines. Useful for debug visualization. */
void BOX_COMPONENT_DrawWorldBox(BoxComponent* bc, Color color)
{
    assert(bc != NULL);
    DrawBoundingBox(bc->worldBox, color);
}
