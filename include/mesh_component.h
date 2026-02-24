#pragma once

#include "scene_component.h"
#include "raylib.h"

typedef struct MeshComponent MeshComponent;

struct MeshComponent
{
    SceneComponent scene;
    Mesh* mesh;
    Material* material;
    Color tint;
    bool visible;
    BoundingBox localBB;
};

MeshComponent* MESH_COMPONENT_Create(Actor* owner, Mesh* mesh, Material* material);
void MESH_COMPONENT_Draw(MeshComponent* mc);
void MESH_COMPONENT_SetVisible(MeshComponent* mc, bool visible);
void MESH_COMPONENT_SetTint(MeshComponent* mc, Color tint);

BoundingBox MESH_COMPONENT_GetWorldBB(MeshComponent* mc);
