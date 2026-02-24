#include "mesh_component.h"
#include "actor.h"
#include "game.h"
#include "renderer.h"
#include <assert.h>
#include <stdlib.h>

static void MeshComponentDestroy(Component* self)
{
    MeshComponent* mc = (MeshComponent*)self;
    if(!mc) return;
    RENDERER_RemoveMesh(&mc->scene.base.owner->game->renderer, mc);
}

MeshComponent* MESH_COMPONENT_Create(Actor* owner, Mesh* mesh, Material* material)
{
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
	assert(mc != NULL);

    if(mc->type == COMPONENT_TYPE_MESH)
    {
        if (!mc->visible || !mc->mesh || !mc->material) return;

        SCENE_COMPONENT_ComputeWorldTransform(&mc->scene);
        
        Color original = mc->material->maps[MATERIAL_MAP_DIFFUSE].color;
        mc->material->maps[MATERIAL_MAP_DIFFUSE].color = mc->tint;
        
        DrawMesh(*mc->mesh, *mc->material, mc->scene.worldTransform);
        
        mc->material->maps[MATERIAL_MAP_DIFFUSE].color = original;
    }
}

void MESH_COMPONENT_SetVisible(MeshComponent* mc, bool visible)
{

}

void MESH_COMPONENT_SetTint(MeshComponent* mc, Color tint)
{

}

BoundingBox MESH_COMPONENT_GetWorldBB(MeshComponent* mc)
{

}
