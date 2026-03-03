/*******************************************************************************************
*
*   sphere_component.c — Sphere Component Implementation
*
********************************************************************************************/
#include "sphere_component.h"
#include "actor.h"
#include "game.h"
#include "physics_world.h"
#include "memory.h"
#include <assert.h>
#include <math.h>

/*------------------------------------------------------------------------------------
 * SPHERE_COMPONENT_Update (static)
 * 
 *   Per-tick update: recomputes world center and world radius from the actor's
 *   transform.
 * 
 *   World center:
 *       Transform the local offset by the actor's world matrix using Vector3Transform.
 * 
 *   World radius (accounting for scale):
 *       The actor might be scaled non-uniformly (e.g., scale = {2, 1, 3}).
 *       A sphere must remain spherical, so we use the MAXIMUM scale axis.
 *       The scale per axis is extracted from the world matrix as the length
 *       of each column vector:
 *           scaleX = length(column 0)  = |(m0, m1, m2)|
 *           scaleY = length(column 1)  = |(m4, m5, m6)|
 *           scaleZ = length(column 2)  = |(m8, m9, m10)|
 *       worldRadius = localRadius × max(scaleX, scaleY, scaleZ)
 * 
 *       This is a conservative approach — the sphere always fully encloses
 *       the original local sphere even under non-uniform scaling.
 *------------------------------------------------------------------------------------*/
static void SPHERE_COMPONENT_Update(Component* self, float deltaTime)
{
    assert(self != NULL);
    (void)deltaTime;

    if(self->type != COMPONENT_TYPE_SPHERE) return;
    SphereComponent* sc = (SphereComponent*)self;
    Actor* owner = sc->base.owner;

    SCENE_COMPONENT_ComputeWorldTransform(&owner->root);

    /* Transform local offset to world space */
    sc->worldCenter = Vector3Transform(sc->offset, owner->root.worldTransform);

    /* ── Extract per-axis scale from world matrix columns ── */
    Vector3 sx = { owner->root.worldTransform.m0, owner->root.worldTransform.m1, owner->root.worldTransform.m2 };
    Vector3 sy = { owner->root.worldTransform.m4, owner->root.worldTransform.m5, owner->root.worldTransform.m6 };
    Vector3 sz = { owner->root.worldTransform.m8, owner->root.worldTransform.m9, owner->root.worldTransform.m10 };
    float scaleX = Vector3Length(sx);
    float scaleY = Vector3Length(sy);
    float scaleZ = Vector3Length(sz);

    /* Use max scale to keep the sphere conservative */
    float maxScale = fmaxf(scaleX, fmaxf(scaleY, scaleZ));

    sc->worldRadius = sc->radius * maxScale;
}

/*------------------------------------------------------------------------------------
 * SPHERE_COMPONENT_Destroy (static)
 * 
 *   Destroy callback — unregisters this sphere from the PhysWorld's collider list.
 *------------------------------------------------------------------------------------*/
static void SPHERE_COMPONENT_Destroy(Component* self)
{
    assert(self != NULL);
    if(self->type != COMPONENT_TYPE_SPHERE) return;
    SphereComponent* sc = (SphereComponent*)self;
    PHYS_WORLD_RemoveSphere(&sc->base.owner->game->physWorld, sc);
}

/*------------------------------------------------------------------------------------
 * SPHERE_COMPONENT_Create
 * 
 *   Factory function — allocates from the component pool, initializes the base,
 *   sets up callbacks, and registers with the PhysWorld.
 * 
 *   Default: center at origin, radius 0.5 (unit sphere), all collision layers.
 *   Update order 300 matches BoxComponent (colliders update after movement).
 *------------------------------------------------------------------------------------*/
SphereComponent* SPHERE_COMPONENT_Create(Actor* owner)
{
    assert(owner != NULL);

    SphereComponent* sc = (SphereComponent*)MEMORY_AllocComponent(
        &owner->game->memory, sizeof(SphereComponent));
    if (!sc) return NULL;

    COMPONENT_Init(&sc->base, owner, COMPONENT_TYPE_SPHERE, 300);
    sc->base.Update = SPHERE_COMPONENT_Update;
    sc->base.Destroy = SPHERE_COMPONENT_Destroy;

    sc->offset = (Vector3){ 0 };
    sc->radius = 0.5f;
    sc->worldCenter = (Vector3){ 0 };
    sc->worldRadius = 0.5f;
    sc->layerMask = 0xFFFFFFFF;     /* Default: collides with everything */
    sc->isTrigger = false;

    /* Register with physics world for collision queries */
    PHYS_WORLD_AddSphere(&owner->game->physWorld, sc);

    return sc;
}

/* Set local-space offset and radius. Offset is relative to the actor's origin. */
void SPHERE_COMPONENT_Set(SphereComponent* sc, Vector3 offset, float radius)
{
    assert(sc != NULL);
    sc->offset = offset;
    sc->radius = radius;
}

/* Get the cached world-space center (updated each tick). */
Vector3 SPHERE_COMPONENT_GetWorldCenter(SphereComponent* sc)
{
    assert(sc != NULL);
    return sc->worldCenter;
}

/* Get the cached world-space radius (updated each tick, accounts for scale). */
float SPHERE_COMPONENT_GetWorldRadius(SphereComponent* sc)
{
    assert(sc != NULL);
	return sc->worldRadius;
}

/*------------------------------------------------------------------------------------
 * SPHERE_COMPONENT_DrawWires
 * 
 *   Debug visualization — draws the sphere as wireframe rings.
 *   Uses worldCenter and worldRadius so the visualization matches the collision shape.
 *   8 rings × 8 segments provides a reasonable wireframe density for debugging.
 *------------------------------------------------------------------------------------*/
void SPHERE_COMPONENT_DrawWires(SphereComponent* sc, Color color)
{
    assert(sc != NULL);
    DrawSphereWires(sc->worldCenter, sc->worldRadius, 8, 8, color);
}
