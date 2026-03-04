/*******************************************************************************************
*
*   physics_world.h — Physics World (Collision Detection System)
*
*   PhysWorld is the central collision detection system. It maintains lists of all
*   collider components (boxes, spheres) and provides spatial queries:
*
*       - Ray casting (find first hit along a ray)
*       - Overlap tests (find all colliders in a region)
*       - Sphere casting (swept sphere test)
*       - Pairwise collision testing (all-vs-all with callbacks)
*
*   Architecture:
*       Game
*           └── PhysWorld (embedded)
*                   ├── boxes[]   → all BoxComponents in the world
*                   ├── spheres[] → all SphereComponents in the world
*                   └── callbacks → per-pair and per-contact notifications
*
*   Broad Phase Strategies:
*       - TestPairwise:     O(n²) brute force — simple, correct, but slow
*       - TestSweepAndPrune: O(n log n) — sorts on X axis for early exit
*       - (Phase 5): Spatial grid for O(n) average case
*
*   Naming Convention:
*       API:     PHYS_WORLD_*
*
********************************************************************************************/
#pragma once

#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct PhysWorld PhysWorld;
typedef struct Actor Actor;
typedef struct Component Component;
typedef struct BoxComponent BoxComponent;
typedef struct SphereComponent SphereComponent;
typedef struct CapsuleComponent CapsuleComponent;   /* Phase 5 */
typedef struct CollisionInfo CollisionInfo;
typedef struct ContactInfo ContactInfo;             /* Phase 5 */

/* ── Capacity Limits ─────────────────────────────────────────────────────── */
#define PHYS_WORLD_MAX_BOXES   512      /* Max simultaneous box colliders    */
#define PHYS_WORLD_MAX_SPHERES 512      /* Max simultaneous sphere colliders */
#define PHYS_WORLD_MAX_CAPSULES 256     /* Phase 5: capsule colliders        */

/* ── Collision Result ────────────────────────────────────────────────────── */
struct CollisionInfo
{
    bool      hit;              /* True if a collision was detected                  */
    float     distance;         /* Distance from ray origin to hit point             */
    Vector3   point;            /* World-space intersection point                    */
    Vector3   normal;           /* Surface normal at the hit point                   */
    Component* collider;        /* The component that was hit (cast to specific type)*/
    Actor*    actor;            /* The actor that owns the hit collider              */
};

/* ── Collision Callbacks ─────────────────────────────────────────────────── */

/* Legacy pair callback: called for each overlapping pair of actors. */
typedef void (*CollisionPairFn)(Actor* a, Actor* b, Vector3 normal, float penetration);

/* Phase 5: Detailed contact callback with full contact manifold info. */
struct ContactInfo
{
    Actor*    actorA;           /* First actor in the collision pair                  */
    Actor*    actorB;           /* Second actor in the collision pair                 */
    Component* colliderA;      /* Collider component on actor A                      */
    Component* colliderB;      /* Collider component on actor B                      */
    Vector3   contactPoint;    /* World-space contact point                          */
    Vector3   contactNormal;   /* Contact normal pointing from A to B                */
    float     penetration;     /* Overlap depth (positive = overlapping)             */
};

typedef void (*ContactCallbackFn)(const ContactInfo* contact);

/* ── Physics World Struct ────────────────────────────────────────────────── */
struct PhysWorld
{
    BoxComponent* boxes[PHYS_WORLD_MAX_BOXES];          /* Registered box colliders      */
    int boxCount;                                       /* Current number of boxes       */
    SphereComponent* spheres[PHYS_WORLD_MAX_SPHERES];   /* Registered sphere colliders  */
    int sphereCount;                                    /* Current number of spheres     */

    /* CapsuleComponent* capsules[PHYS_WORLD_MAX_CAPSULES]; */  /* Phase 5 */
    /* int capsuleCount; */

    /* Phase 5: collision layer matrix — which layers collide with which */
    /* uint32_t layerMatrix[32]; */

    /* Phase 5: spatial partitioning grid for O(n) broad phase */
    /* SpatialGrid* grid; */

    /* Callbacks */
    ContactCallbackFn onContact;        /* Called for each contact point (Phase 5)  */
    CollisionPairFn onPairCollision;    /* Called for each overlapping pair          */
};

/* ── Lifecycle ───────────────────────────────────────────────────────────── */
void PHYS_WORLD_Init(PhysWorld* world);
void PHYS_WORLD_Shutdown(PhysWorld* world);
void PHYS_WORLD_Update(PhysWorld* world, float deltaTime);

/* ── Collider Registration ───────────────────────────────────────────────── */
void PHYS_WORLD_AddBox(PhysWorld* world, BoxComponent* box);
void PHYS_WORLD_RemoveBox(PhysWorld* world, BoxComponent* box);
void PHYS_WORLD_AddSphere(PhysWorld* world, SphereComponent* sphere);
void PHYS_WORLD_RemoveSphere(PhysWorld* world, SphereComponent* sphere);
/* void PHYS_WORLD_AddCapsule(PhysWorld* world, CapsuleComponent* cap); */
/* void PHYS_WORLD_RemoveCapsule(PhysWorld* world, CapsuleComponent* cap); */

/* ── Spatial Queries ─────────────────────────────────────────────────────── */
bool PHYS_WORLD_RayCast(PhysWorld* world, Ray ray, float maxDist, uint32_t layerMask, CollisionInfo* outHit);
bool PHYS_WORLD_RayCastIgnore(PhysWorld* world, Ray ray, float maxDist, uint32_t layerMask, Actor* ignore, CollisionInfo* outHit);
int PHYS_WORLD_OverlapSphere(PhysWorld* world, Vector3 center, float radius, uint32_t layerMask, Actor** outActors, int maxResults);
int PHYS_WORLD_OverlapBox(PhysWorld* world, BoundingBox box, uint32_t layerMask, Actor** outActors, int maxResults);
bool PHYS_WORLD_SphereCast(PhysWorld* world, Vector3 origin, float radius, Vector3 direction, float maxDist, uint32_t layerMask, CollisionInfo* outHit);

/* ── Collision Testing Algorithms ────────────────────────────────────────── */
void PHYS_WORLD_TestPairwise(PhysWorld* world, CollisionPairFn fn);
void PHYS_WORLD_TestSweepAndPrune(PhysWorld* world, CollisionPairFn fn);

/* void PHYS_WORLD_SetLayerCollision(PhysWorld* world,
                                     int layerA, int layerB, bool collides); */