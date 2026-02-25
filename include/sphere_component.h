#pragma once

/*
 * sphere_component.h — Sphere collider component.
 *
 * SphereComponent extends Component (no scene-graph transform of its own)
 * and registers itself with the PhysWorld.  Each fixed update it transforms
 * the local-space offset through the actor's world matrix and extracts the
 * maximum world-space scale to scale the radius uniformly.
 *
 * Architecture position:
 *   Actor → SphereComponent (updateOrder 300)
 *   PhysWorld.spheres[] → SphereComponent (flat list for broadphase tests)
 */

#include "component.h"
#include "raylib.h"
#include <stdbool.h>

typedef struct Actor Actor;

typedef struct SphereComponent
{
    Component    base;         /* Embedded Component — must be first field for safe casting */
    Vector3      offset;       /* Local-space offset from the actor's origin to the sphere centre */
    float        radius;       /* Sphere radius in local space */
    Vector3      worldCenter;  /* Sphere centre in world space (recomputed each tick) */
    float        worldRadius;  /* Sphere radius in world space scaled by the actor's maximum axis scale */
    unsigned int layerMask;    /* Collision layer bitmask — determines which layers this collider interacts with */
    bool         isTrigger;    /* When true the sphere fires overlap events but does not block movement */
} SphereComponent;

/* ── Public API ─────────────────────────────────────────────────── */

SphereComponent* SPHERE_COMPONENT_Create(Actor* owner);                            // Allocate a SphereComponent, register with the physics world, and attach to owner
void             SPHERE_COMPONENT_Set(SphereComponent* sc, Vector3 offset, float radius); // Set both the local offset and the local radius at once

Vector3 SPHERE_COMPONENT_GetWorldCenter(SphereComponent* sc); // Return the cached world-space sphere centre (updated during Update callback)
float   SPHERE_COMPONENT_GetWorldRadius(SphereComponent* sc); // Return the cached world-space radius (updated during Update callback)

void SPHERE_COMPONENT_DrawWires(SphereComponent* sc, Color color); // Draw the world-space sphere as a wireframe (useful for debug visualisation)