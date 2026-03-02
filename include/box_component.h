/*******************************************************************************************
*
*   box_component.h — Box Component (AABB Collider)
*
*   BoxComponent represents an Axis-Aligned Bounding Box (AABB) collider attached to
*   an actor. It stores a local-space box (objectBox) that gets transformed to
*   world space (worldBox) every frame based on the actor's root transform.
*
*   Integration:
*       Actor.root (SceneComponent)  →  transform source
*       BoxComponent                 →  collider (uses root's worldTransform)
*           ├── Registered with PhysWorld on Create
*           └── Unregistered from PhysWorld on Destroy
*
*   The PhysWorld uses these for collision detection (ray casts, overlap tests,
*   and pairwise collision checks).
*
*   Naming Convention:
*       API:     BOX_COMPONENT_*
*
********************************************************************************************/
#pragma once

#include "component.h"
#include "raylib.h"
#include <stdbool.h>

typedef struct BoxComponent BoxComponent;
typedef struct Actor Actor;

/* ── Box Component Struct ────────────────────────────────────────────────── */
struct BoxComponent
{
    Component base;                 /* Inherited base component (must be first field)  */
    BoundingBox objectBox;          /* AABB in local/object space                      */
    BoundingBox worldBox;           /* AABB transformed to world space (cached)        */
    unsigned int layerMask;         /* Phase 5: collision layer bitmask for filtering  */
    bool isTrigger;                 /* Phase 5: if true, overlaps but doesn't block    */
};

/* ── Public API ──────────────────────────────────────────────────────────── */
BoxComponent* BOX_COMPONENT_Create(Actor* owner);
void BOX_COMPONENT_SetObjectBox(BoxComponent* bc, BoundingBox objectBox);
void BOX_COMPONENT_SetFromMesh(BoxComponent* bc, Mesh mesh);
BoundingBox BOX_COMPONENT_GetWorldBox(BoxComponent* bc);
void BOX_COMPONENT_DrawWorldBox(BoxComponent* bc, Color color);