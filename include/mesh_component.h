#pragma once

/*
 * mesh_component.h — Renderable geometry component.
 *
 * MeshComponent extends SceneComponent so it participates in the scene
 * graph, inheriting world-space position/rotation/scale from its parent
 * (normally the actor's root).  It pairs a Raylib Mesh with a Material and
 * registers itself with the Renderer's draw list on creation.
 *
 * On each frame the Renderer performs frustum culling against localBB
 * transformed to world-space (8-corner AABB transform), then draws visible
 * entries.
 *
 * Architecture position:
 *   Actor.root → MeshComponent.scene (child SceneComponent)
 *   Renderer.meshes[] → MeshComponent (flat list for culling/drawing)
 */

#include "scene_component.h"
#include "raylib.h"

typedef struct MeshComponent MeshComponent;

/* ── MeshComponent Struct ───────────────────────────────────────── */

struct MeshComponent
{
    SceneComponent scene;   /* Embedded SceneComponent — provides world transform and hierarchy attachment */
    Mesh*          mesh;    /* Pointer to the Raylib Mesh geometry (not owned; must outlive component) */
    Material*      material;/* Pointer to the Raylib Material used for rendering (not owned) */
    Color          tint;    /* Per-instance colour tint applied to the diffuse map at draw time */
    bool           visible; /* When false the mesh is skipped entirely during build-draw-list */
    BoundingBox    localBB; /* Local-space AABB computed from mesh at creation; transformed to world for culling */
};

/* ── Public API ─────────────────────────────────────────────────── */

MeshComponent* MESH_COMPONENT_Create(Actor* owner, Mesh* mesh, Material* material); // Allocate, initialise, attach to scene graph, and register with the renderer
void MESH_COMPONENT_Draw(MeshComponent* mc);                                         // Draw the mesh using its world transform and tinted material
void MESH_COMPONENT_SetVisible(MeshComponent* mc, bool visible);                     // Show or hide the mesh (hidden meshes skip frustum cull and draw)
void MESH_COMPONENT_SetTint(MeshComponent* mc, Color tint);                          // Set the per-instance diffuse colour tint

BoundingBox MESH_COMPONENT_GetWorldBB(MeshComponent* mc); // Return the world-space AABB by transforming localBB with the current world transform
