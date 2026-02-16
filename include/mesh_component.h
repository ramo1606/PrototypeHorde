#pragma once

#include "component.h"
#include "raylib.h"

typedef struct 
{
    Component base;
    Mesh* mesh;
    Material* material;
	Color tint;
    bool visible;
} MeshComponent;

MeshComponent* MESH_COMPONENT_Create(Actor* owner, Mesh* mesh, Material* material);
void MESH_COMPONENT_Draw(Component* mc);
