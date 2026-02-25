#include "sphere_component.h"
#include "actor.h"
#include "game.h"
#include "physics_world.h"
#include "memory.h"
#include <assert.h>
#include <math.h>

static void SphereComponentUpdate(Component* self, float deltaTime)
{
    /*
     * Recompute the world-space sphere each fixed tick.
     *
     * World centre: transform the local offset point through the actor's
     *   world matrix.
     *
     * World radius: extract the scale columns from the world matrix and
     *   take the maximum magnitude across all three axes.  This ensures
     *   the world radius encloses the sphere under non-uniform scale
     *   (conservative but always correct).
     *
     *   Column 0 (m0,m1,m2)  = world X axis (scale embedded)
     *   Column 1 (m4,m5,m6)  = world Y axis
     *   Column 2 (m8,m9,m10) = world Z axis
     */
    (void)deltaTime;
    SphereComponent* sc = (SphereComponent*)self;
    Actor* owner = sc->base.owner;

    SCENE_COMPONENT_ComputeWorldTransform(&owner->root);

    sc->worldCenter = Vector3Transform(sc->offset, owner->root.worldTransform);

    Vector3 sx = { owner->root.worldTransform.m0, owner->root.worldTransform.m1, owner->root.worldTransform.m2 };
    Vector3 sy = { owner->root.worldTransform.m4, owner->root.worldTransform.m5, owner->root.worldTransform.m6 };
    Vector3 sz = { owner->root.worldTransform.m8, owner->root.worldTransform.m9, owner->root.worldTransform.m10 };
    float scaleX = Vector3Length(sx);
    float scaleY = Vector3Length(sy);
    float scaleZ = Vector3Length(sz);
    float maxScale = fmaxf(scaleX, fmaxf(scaleY, scaleZ));

    sc->worldRadius = sc->radius * maxScale;
}

static void SphereComponentDestroy(Component* self)
{
    /*
     * Unregister from the physics world before memory is freed so the
     * world's sphere list never contains a dangling pointer.
     */
    SphereComponent* sc = (SphereComponent*)self;
    PHYS_WORLD_RemoveSphere(&sc->base.owner->game->physWorld, sc);
}

SphereComponent* SPHERE_COMPONENT_Create(Actor* owner)
{
    /*
     * Allocate from the component pool, register at updateOrder 300
     * (same priority as BoxComponent so all colliders are updated in the
     * same pass), and register with the physics world.
     * Default layerMask = 0xFFFFFFFF collides with all layers.
     */
    assert(owner != NULL);

    SphereComponent* sc = (SphereComponent*)MEMORY_AllocComponent(
        &owner->game->memory, sizeof(SphereComponent));
    if (!sc) return NULL;

    COMPONENT_Init(&sc->base, owner, COMPONENT_TYPE_SPHERE, 300);
    sc->base.Update = SphereComponentUpdate;
    sc->base.Destroy = SphereComponentDestroy;

    sc->offset = (Vector3){ 0 };
    sc->radius = 0.5f;
    sc->worldCenter = (Vector3){ 0 };
    sc->worldRadius = 0.5f;
    sc->layerMask = 0xFFFFFFFF;     /* Default: collides with everything */
    sc->isTrigger = false;

    PHYS_WORLD_AddSphere(&owner->game->physWorld, sc);

    return sc;
}

void SPHERE_COMPONENT_Set(SphereComponent* sc, Vector3 offset, float radius)
{
    assert(sc != NULL);
    sc->offset = offset;
    sc->radius = radius;
}

Vector3 SPHERE_COMPONENT_GetWorldCenter(SphereComponent* sc)
{
    assert(sc != NULL);
    return sc->worldCenter;
}

float SPHERE_COMPONENT_GetWorldRadius(SphereComponent* sc)
{
    assert(sc != NULL);
	return sc->worldRadius;
}

void SPHERE_COMPONENT_DrawWires(SphereComponent* sc, Color color)
{
    assert(sc != NULL);
    DrawSphereWires(sc->worldCenter, sc->worldRadius, 8, 8, color);
}
