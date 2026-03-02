/*******************************************************************************************
*
*   mesh_component.h — Mesh Component (Renderable Geometry)
*
*   MeshComponent gives an actor visual representation by rendering a Raylib Mesh
*   with a Material. It inherits from SceneComponent (has its own transform) and
*   attaches as a child of the actor's root, so it inherits the actor's position.
*
*   Integration:
*       Actor.root (SceneComponent)
*           └── MeshComponent.scene (child SceneComponent)
*                   ├── mesh*      → Raylib Mesh (geometry data)
*                   └── material*  → Raylib Material (shader, textures, colors)
*
*       On Create: registers with Renderer (added to draw list)
*       On Destroy: unregisters from Renderer
*
*   The Renderer iterates all registered MeshComponents each frame, performs
*   frustum culling against the world bounding box, sorts by material, and draws.
*
*   Naming Convention:
*       API:     MESH_COMPONENT_*
*
********************************************************************************************/
#pragma once

#include "scene_component.h"
#include "raylib.h"

typedef struct MeshComponent MeshComponent;

/* ── Mesh Component Struct ───────────────────────────────────────────────── */
struct MeshComponent
{
    SceneComponent scene;       /* Inherited scene component (must be first field)   */
    Mesh* mesh;                 /* Pointer to the mesh geometry (shared, not owned)  */
    Material* material;         /* Pointer to the material (shared, not owned)       */
    Color tint;                 /* Color multiplier applied to the diffuse map       */
    bool visible;               /* If false, skipped during rendering                */
    BoundingBox localBB;        /* Axis-aligned bounding box in object/local space   */
};

/* ── Public API ──────────────────────────────────────────────────────────── */
MeshComponent* MESH_COMPONENT_Create(Actor* owner, Mesh* mesh, Material* material);
void MESH_COMPONENT_Draw(MeshComponent* mc);
void MESH_COMPONENT_SetVisible(MeshComponent* mc, bool visible);
void MESH_COMPONENT_SetTint(MeshComponent* mc, Color tint);
BoundingBox MESH_COMPONENT_GetWorldBB(MeshComponent* mc);
