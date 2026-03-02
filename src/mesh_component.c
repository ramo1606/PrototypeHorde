/*******************************************************************************************
*
*   mesh_component.c — Mesh Component Implementation
*
********************************************************************************************/
#include "mesh_component.h"
#include "actor.h"
#include "collision.h"
#include "game.h"
#include "renderer.h"
#include <assert.h>
#include <stdlib.h>

/*------------------------------------------------------------------------------------
 * MeshComponentDestroy (static)
 *
 *   Destroy callback — unregisters this mesh from the Renderer's draw list.
 *   The mesh and material pointers are NOT freed here because they are shared
 *   resources.
 *------------------------------------------------------------------------------------*/
static void MESH_COMPONENT_Destroy(Component* self)
{
	assert(self != NULL);
    if(self->type != COMPONENT_TYPE_MESH) return;
    MeshComponent* mc = (MeshComponent*)self;
    if(!mc) return;
    RENDERER_RemoveMesh(&mc->scene.base.owner->game->renderer, mc);
}

/*------------------------------------------------------------------------------------
 * MESH_COMPONENT_Create
 *
 *   Factory function — allocates from the component pool, initializes the
 *   SceneComponent base, attaches as child of the actor's root, computes the
 *   local bounding box from the mesh, and registers with the Renderer.
 *
 *   Update order 200 means MeshComponent updates after movement (10) but before
 *   colliders (300), ensuring the visual transform is current.
 *------------------------------------------------------------------------------------*/
MeshComponent* MESH_COMPONENT_Create(Actor* owner, Mesh* mesh, Material* material)
{
    assert(owner != NULL);
    assert(mesh != NULL);
    assert(material != NULL);

    MeshComponent* self = (MeshComponent*)MEMORY_AllocComponent(&owner->game->memory, sizeof(MeshComponent));
    if (!self) return NULL;

    /* Initialize SceneComponent base and register with Actor */
    SCENE_COMPONENT_Init(&self->scene, owner, COMPONENT_TYPE_MESH, 200);
	self->scene.base.Destroy = MESH_COMPONENT_Destroy;

    /* Attach to actor root so this mesh inherits the actor's transform */
    SCENE_COMPONENT_AttachChild(&owner->root, &self->scene);

    self->mesh = mesh;
    self->material = material;
	self->tint = WHITE;
    self->visible = true;

    /* Compute local AABB from mesh vertices — used for frustum culling */
    self->localBB = GetMeshBoundingBox(*mesh);

    /* Register with the Renderer for automatic drawing */
    RENDERER_AddMesh(&owner->game->renderer, self);

    return self;
}

/*------------------------------------------------------------------------------------
 * MESH_COMPONENT_Draw
 *
 *   Renders the mesh at its world transform position. Called by the Renderer
 *   during the draw pass.
 *
 *   Tinting technique:
 *   Raylib's DrawMesh uses the material's diffuse color as the base tint.
 *   We temporarily swap in our custom tint color, draw, then restore the original.
 *   This allows per-instance coloring without duplicating materials.
 *------------------------------------------------------------------------------------*/
void MESH_COMPONENT_Draw(MeshComponent* mc)
{
	assert(mc != NULL);

    if (!mc->visible || !mc->mesh || !mc->material) return;

    /* Ensure world transform is current */
    SCENE_COMPONENT_ComputeWorldTransform(&mc->scene);
    
    /* ── Tint application: swap diffuse color temporarily ── */
    Color original = mc->material->maps[MATERIAL_MAP_DIFFUSE].color;
    mc->material->maps[MATERIAL_MAP_DIFFUSE].color = mc->tint;
    
    DrawMesh(*mc->mesh, *mc->material, mc->scene.worldTransform);
    
    /* Restore original material color */
    mc->material->maps[MATERIAL_MAP_DIFFUSE].color = original;
}

/* Set mesh visibility — invisible meshes are skipped by the Renderer. */
void MESH_COMPONENT_SetVisible(MeshComponent* mc, bool visible)
{
    assert(mc != NULL);
    mc->visible = visible;
}

/* Set the color tint. WHITE = no tinting (default). */
void MESH_COMPONENT_SetTint(MeshComponent* mc, Color tint)
{
    assert(mc != NULL);
    mc->tint = tint;
}

/*------------------------------------------------------------------------------------
 * MESH_COMPONENT_GetWorldBB
 *
 *   Transforms the local bounding box into world space using the scene's
 *   world transform matrix.
 *
 *   Note: For correct world-space AABBs under
 *   rotation, use COLLISION_TransformAABB which tests all 8 corners.
 *------------------------------------------------------------------------------------*/
BoundingBox MESH_COMPONENT_GetWorldBB(MeshComponent* mc)
{
    assert(mc != NULL);
    SCENE_COMPONENT_ComputeWorldTransform(&mc->scene);
    return COLLISION_TransformAABB(mc->localBB, mc->scene.worldTransform);
}
