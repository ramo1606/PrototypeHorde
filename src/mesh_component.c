#include "mesh_component.h"
#include "actor.h"
#include "collision.h"
#include "game.h"
#include "renderer.h"
#include <assert.h>
#include <stdlib.h>

static void MeshComponentDestroy(Component* self)
{
    /*
     * Called by COMPONENT_Destroy before memory is freed.
     * Unregisters the mesh from the renderer's draw list so it is no
     * longer considered during frustum cull or draw passes.
     */
    MeshComponent* mc = (MeshComponent*)self;
    if(!mc) return;
    RENDERER_RemoveMesh(&mc->scene.base.owner->game->renderer, mc);
}

MeshComponent* MESH_COMPONENT_Create(Actor* owner, Mesh* mesh, Material* material)
{
    /*
     * Allocate from the component MemPool, initialise the embedded
     * SceneComponent (which attaches it to the actor's scene graph),
     * and register with the renderer so it is included in future draw lists.
     *
     * The local bounding box is computed once from the mesh at creation
     * time and stored in localBB.  Each frame the renderer transforms it
     * to world-space for frustum culling using COLLISION_TransformAABB.
     */
    assert(owner != NULL);
    assert(mesh != NULL);
    assert(material != NULL);

    MeshComponent* self = (MeshComponent*)MEMORY_AllocComponent(&owner->game->memory, sizeof(MeshComponent));
    if (!self) return NULL;

    SCENE_COMPONENT_Init(&self->scene, owner, COMPONENT_TYPE_MESH, 200);
	self->scene.base.Destroy = MeshComponentDestroy;
    SCENE_COMPONENT_AttachChild(&owner->root, &self->scene);

    self->mesh = mesh;
    self->material = material;
	self->tint = WHITE;
    self->visible = true;
    self->localBB = GetMeshBoundingBox(*mesh);

    RENDERER_AddMesh(&owner->game->renderer, self);

    return self;
}

void MESH_COMPONENT_Draw(MeshComponent* mc)
{
    /*
     * Draw the mesh at its current world transform.
     *
     * The tint colour is applied by temporarily replacing the material's
     * diffuse map colour, drawing, then restoring the original colour.
     * This avoids a material copy and keeps the material shared.
     */
	assert(mc != NULL);

    if (!mc->visible || !mc->mesh || !mc->material) return;

    SCENE_COMPONENT_ComputeWorldTransform(&mc->scene);
    
    Color original = mc->material->maps[MATERIAL_MAP_DIFFUSE].color;
    mc->material->maps[MATERIAL_MAP_DIFFUSE].color = mc->tint;
    
    DrawMesh(*mc->mesh, *mc->material, mc->scene.worldTransform);
    
    mc->material->maps[MATERIAL_MAP_DIFFUSE].color = original;
}

void MESH_COMPONENT_SetVisible(MeshComponent* mc, bool visible)
{
    assert(mc != NULL);
    mc->visible = visible;
}

void MESH_COMPONENT_SetTint(MeshComponent* mc, Color tint)
{
    assert(mc != NULL);
    mc->tint = tint;
}

BoundingBox MESH_COMPONENT_GetWorldBB(MeshComponent* mc)
{
    /*
     * AABB transform via the 8-corner method (see COLLISION_TransformAABB).
     * Ensures the world bounding box is always axis-aligned and correct
     * under rotation and non-uniform scale.
     */
    assert(mc != NULL);
    SCENE_COMPONENT_ComputeWorldTransform(&mc->scene);
    return COLLISION_TransformAABB(mc->localBB, mc->scene.worldTransform);
}
