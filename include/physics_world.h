#pragma once

#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>

typedef struct Actor Actor;
typedef struct Component Component;
typedef struct BoxComponent BoxComponent;
typedef struct SphereComponent SphereComponent;

#define PHYS_WORLD_MAX_BOXES   512
#define PHYS_WORLD_MAX_SPHERES 512

 /* Collision result from RayCast */
typedef struct
{
    RayCollision collision;
    Component* collider;    /* Component that was hit (cast via ->type) */
    Actor* actor;       /* Actor that owns the component */
} CollisionInfo;

/* Callback for pairwise collision tests */
typedef void (*CollisionPairFn)(Actor* a, Actor* b);

typedef struct
{
    BoxComponent* boxes[PHYS_WORLD_MAX_BOXES];
    int boxCount;
    SphereComponent* spheres[PHYS_WORLD_MAX_SPHERES];
    int sphereCount;
} PhysWorld;

/* Lifecycle */
void PHYS_WORLD_Init(PhysWorld* world);

/* Registration (called by component create/destroy) */
void PHYS_WORLD_AddBox(PhysWorld* world, BoxComponent* box);
void PHYS_WORLD_RemoveBox(PhysWorld* world, BoxComponent* box);
void PHYS_WORLD_AddSphere(PhysWorld* world, SphereComponent* sphere);
void PHYS_WORLD_RemoveSphere(PhysWorld* world, SphereComponent* sphere);

/* Queries */

/* Cast a ray against all colliders up to maxDistance.
 * Returns true if any hit, outColl receives the closest. */
bool PHYS_WORLD_RayCast(PhysWorld* world, Ray ray, float maxDistance,
    CollisionInfo* outColl);

/* O(n^2) pairwise test — tests all type combinations.
 * Calls fn for each intersecting pair. */
void PHYS_WORLD_TestPairwise(PhysWorld* world, CollisionPairFn fn);

/* Sweep-and-prune on X axis — box-vs-box only (optimization). */
void PHYS_WORLD_TestSweepAndPrune(PhysWorld* world, CollisionPairFn fn);