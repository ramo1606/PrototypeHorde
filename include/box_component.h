#pragma once

/*
 * box_component.h — Axis-aligned bounding box (AABB) collider component.
 *
 * BoxComponent extends Component (no scene-graph transform of its own) and
 * registers itself with the PhysWorld.  Each fixed update it re-transforms
 * its local-space objectBox through the owner actor's world transform to
 * produce the world-space worldBox used for collision tests.
 *
 * The 8-corner AABB transform technique is used (see collision.h /
 * COLLISION_TransformAABB) so the box stays correct under non-uniform
 * scale and rotation.
 *
 * Architecture position:
 *   Actor → BoxComponent (updateOrder 300)
 *   PhysWorld.boxes[] → BoxComponent (flat list for broadphase tests)
 */

#include "component.h"
#include "raylib.h"
#include <stdbool.h>

typedef struct BoxComponent BoxComponent;
typedef struct Actor Actor;

/* ── BoxComponent Struct ────────────────────────────────────────── */

struct BoxComponent
{
    Component   base;           /* Embedded Component — must be first field for safe casting */
    BoundingBox objectBox;      /* AABB in local object space (set from mesh or manually) */
    BoundingBox worldBox;       /* AABB in world space (recomputed each tick from objectBox * worldTransform) */
    unsigned int layerMask;     /* Phase 5: collision layer */
    bool isTrigger;             /* Phase 5: overlap only, no blocking */
};

/* ── Public API ─────────────────────────────────────────────────── */

BoxComponent* BOX_COMPONENT_Create(Actor* owner);                              // Allocate a BoxComponent, register it with the physics world, and attach to the owner
void          BOX_COMPONENT_SetObjectBox(BoxComponent* bc, BoundingBox objectBox); // Manually set the local-space AABB
void          BOX_COMPONENT_SetFromMesh(BoxComponent* bc, Mesh mesh);          // Derive the local-space AABB from a Raylib Mesh's bounding box

BoundingBox BOX_COMPONENT_GetWorldBox(BoxComponent* bc);              // Return the cached world-space AABB (updated during component's Update callback)
void        BOX_COMPONENT_DrawWorldBox(BoxComponent* bc, Color color); // Draw the world-space AABB as wireframe lines (useful for debug visualisation)