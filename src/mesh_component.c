#include "mesh_component.h"
#include "actor.h"
#include <assert.h>
#include <stdlib.h>

MeshComponent* MESH_COMPONENT_Create(Actor* owner, Mesh* mesh, Material* material)
{
    assert(owner != NULL);
    assert(mesh != NULL);
    assert(material != NULL);

	//TODO: replace for actual memory management system
    MeshComponent* self = malloc(sizeof(MeshComponent));
    if (!self) return NULL;

    COMPONENT_Init(&self->base, owner, COMPONENT_MESH, 200);
    self->mesh = mesh;
    self->material = material;
	self->tint = WHITE;
    self->visible = true;

    return self;
}

void MESH_COMPONENT_Draw(Component* mc)
{
	assert(mc != NULL);

    if(mc->type == COMPONENT_MESH)
    {
        MeshComponent* meshComponent = (MeshComponent*)mc;
        if (!meshComponent->visible || !meshComponent->mesh || !meshComponent->material) return;
        
        /* Save original diffuse color, apply per-component tint */
        Color original = meshComponent->material->maps[MATERIAL_MAP_DIFFUSE].color;
        meshComponent->material->maps[MATERIAL_MAP_DIFFUSE].color = meshComponent->tint;

        DrawMesh(*meshComponent->mesh, *meshComponent->material, meshComponent->base.owner->worldTransform);
        /* Restore shared material color */
        meshComponent->material->maps[MATERIAL_MAP_DIFFUSE].color = original;
    }
}
