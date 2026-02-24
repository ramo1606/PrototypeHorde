#include "sphere_component.h"
#include "actor.h"
#include "game.h"
#include "physics_world.h"
#include "memory.h"
#include <assert.h>
#include <math.h>

static void SphereComponentUpdate(Component* self, float deltaTime)
{
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
    SphereComponent* sc = (SphereComponent*)self;
    PHYS_WORLD_RemoveSphere(&sc->base.owner->game->physWorld, sc);
}

SphereComponent* SPHERE_COMPONENT_Create(Actor* owner)
{
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

float SPHERE_COMPONENT_GetRadius(SphereComponent* sc)
{
    assert(sc != NULL);
    return sc->radius;
}

void SPHERE_COMPONENT_DrawWires(SphereComponent* sc, Color color)
{
    assert(sc != NULL);
    DrawSphereWires(sc->worldCenter, sc->radius, 8, 8, color);
}
