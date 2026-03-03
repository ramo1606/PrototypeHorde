
/*******************************************************************************************
*
*   sphere_component.h — Sphere Component (Sphere Collider)
*
*   SphereComponent represents a sphere collider attached to an actor. Unlike
*   BoxComponent, it doesn't inherit from SceneComponent — it reads the actor's
*   root transform directly to compute its world center and radius.
*
*   Integration:
*       Actor.root (SceneComponent)  →  transform source
*       SphereComponent              →  collider
*           ├── Registered with PhysWorld on Create
*           └── Unregistered from PhysWorld on Destroy
*
*   The world radius is computed using the maximum scale axis of the actor's
*   transform, so the sphere always fully encloses the local sphere even under
*   non-uniform scaling.
*
*   Naming Convention:
*       API:     SPHERE_COMPONENT_*
*
********************************************************************************************/#pragma once
#include "component.h"
#include "raylib.h"
#include <stdbool.h>

typedef struct Actor Actor;
typedef struct SphereComponent SphereComponent;

/* ── Sphere Component Struct ─────────────────────────────────────────────── */
struct SphereComponent
{
    Component base;             /* Inherited base component (must be first field)    */
    Vector3 offset;             /* Center offset in local/object space               */
    float radius;               /* Radius in local/object space                      */
    Vector3 worldCenter;        /* Center transformed to world space (cached)        */
    float worldRadius;          /* Radius scaled to world space (cached)             */
    unsigned int layerMask;     /* Phase 5: collision layer bitmask for filtering    */
    bool isTrigger;             /* Phase 5: if true, overlaps but doesn't block      */
};

/* ── Public API ──────────────────────────────────────────────────────────── */
SphereComponent* SPHERE_COMPONENT_Create(Actor* owner);
void SPHERE_COMPONENT_Set(SphereComponent* sc, Vector3 offset, float radius);
Vector3 SPHERE_COMPONENT_GetWorldCenter(SphereComponent* sc);
float SPHERE_COMPONENT_GetWorldRadius(SphereComponent* sc);
void SPHERE_COMPONENT_DrawWires(SphereComponent* sc, Color color);