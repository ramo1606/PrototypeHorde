#pragma once

/*
 * physics_world.h — Broadphase collision detection and spatial queries.
 *
 * PhysWorld manages flat arrays of BoxComponent and SphereComponent
 * pointers.  Each fixed update it runs broadphase collision detection and
 * fires pair callbacks.  Two strategies are available:
 *
 *   - TestPairwise: brute-force O(n²) over all box/sphere pairs — simple
 *     and correct for small scenes.
 *   - TestSweepAndPrune: sort boxes by min.x then early-exit when the
 *     sweep interval no longer overlaps — reduces box-box tests to O(n log n)
 *     + O(k) where k is the number of actual overlaps.
 *
 * Spatial queries (RayCast, OverlapSphere, OverlapBox, SphereCast) are
 * brute-force against all registered colliders.
 *
 * Architecture position:
 *   Game.physWorld (value, not pointer)
 *   BoxComponent / SphereComponent register/unregister themselves on
 *   create/destroy.
 */

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

/* ── Capacity Constants ─────────────────────────────────────────── */

#define PHYS_WORLD_MAX_BOXES   512   /* Maximum number of BoxComponents that can be registered */
#define PHYS_WORLD_MAX_SPHERES 512   /* Maximum number of SphereComponents that can be registered */
#define PHYS_WORLD_MAX_CAPSULES 256  /* Phase 5 */

/* ── Query Result Structs ───────────────────────────────────────── */

struct CollisionInfo
{
    bool       hit;       /* True when a collision was detected */
    float      distance;  /* Distance from ray origin to the closest hit point */
    Vector3    point;     /* World-space position of the closest hit point */
    Vector3    normal;    /* Surface normal at the hit point */
    Component* collider;  /* The collider component that was hit */
    Actor*     actor;     /* The actor that owns the hit collider */
};

typedef void (*CollisionPairFn)(Actor* a, Actor* b, Vector3 normal, float penetration); /* Callback fired for each overlapping pair */

/* Phase 5: detailed collision callback with full contact info */
struct ContactInfo
{
    Actor*     actorA;         /* First actor in the contact pair */
    Actor*     actorB;         /* Second actor in the contact pair */
    Component* colliderA;      /* Collider component belonging to actorA */
    Component* colliderB;      /* Collider component belonging to actorB */
    Vector3    contactPoint;   /* World-space contact point (typically midpoint of overlap) */
    Vector3    contactNormal;  /* Normal pointing from actorA to actorB */
    float      penetration;    /* Overlap depth along contactNormal */
};

typedef void (*ContactCallbackFn)(const ContactInfo* contact); /* Detailed contact callback (Phase 5) */

/* ── PhysWorld Struct ───────────────────────────────────────────── */

struct PhysWorld
{
    BoxComponent* boxes[PHYS_WORLD_MAX_BOXES];        /* Registered AABB colliders */
    int           boxCount;                           /* Number of active entries in boxes[] */
    SphereComponent* spheres[PHYS_WORLD_MAX_SPHERES]; /* Registered sphere colliders */
    int           sphereCount;                        /* Number of active entries in spheres[] */

    /* CapsuleComponent* capsules[PHYS_WORLD_MAX_CAPSULES]; */  /* Phase 5 */
    /* int capsuleCount; */

    /* Phase 5: collision layer matrix */
    /* uint32_t layerMatrix[32]; */

    /* Phase 5: spatial grid */
    /* SpatialGrid* grid; */

    /* Callbacks */
    ContactCallbackFn  onContact;          /* Called per contact */
    CollisionPairFn    onPairCollision;    /* Legacy pair callback */
};

/* ── Lifecycle ──────────────────────────────────────────────────── */

void PHYS_WORLD_Init(PhysWorld* world);                            // Initialise all arrays and callbacks to zero/NULL
void PHYS_WORLD_Shutdown(PhysWorld* world);                        // Reset counts and clear callbacks (colliders de-register themselves via component Destroy)
void PHYS_WORLD_Update(PhysWorld* world, float deltaTime);         // Run the active broadphase pass and fire pair/contact callbacks

/* ── Collider Registration ──────────────────────────────────────── */

void PHYS_WORLD_AddBox(PhysWorld* world, BoxComponent* box);             // Register a BoxComponent for broadphase tests
void PHYS_WORLD_RemoveBox(PhysWorld* world, BoxComponent* box);          // Unregister a BoxComponent (swap-remove, O(n))
void PHYS_WORLD_AddSphere(PhysWorld* world, SphereComponent* sphere);    // Register a SphereComponent for broadphase tests
void PHYS_WORLD_RemoveSphere(PhysWorld* world, SphereComponent* sphere); // Unregister a SphereComponent (swap-remove, O(n))
/* void PHYS_WORLD_AddCapsule(PhysWorld* world, CapsuleComponent* cap); */
/* void PHYS_WORLD_RemoveCapsule(PhysWorld* world, CapsuleComponent* cap); */

/* ── Spatial Queries ────────────────────────────────────────────── */

bool PHYS_WORLD_RayCast(PhysWorld* world, Ray ray, float maxDist, uint32_t layerMask,
    CollisionInfo* outHit);  // Cast a ray against all registered colliders; returns true and fills outHit with the closest result

bool PHYS_WORLD_RayCastIgnore(PhysWorld* world, Ray ray, float maxDist, uint32_t layerMask,
    Actor* ignore, CollisionInfo* outHit);  // Same as RayCast but skips all colliders owned by ignore actor

int PHYS_WORLD_OverlapSphere(PhysWorld* world, Vector3 center, float radius,
    uint32_t layerMask, Actor** outActors, int maxResults); // Return all actors whose colliders overlap the given sphere (up to maxResults)

int PHYS_WORLD_OverlapBox(PhysWorld* world, BoundingBox box,
    uint32_t layerMask, Actor** outActors, int maxResults); // Return all actors whose colliders overlap the given AABB (up to maxResults)

bool PHYS_WORLD_SphereCast(PhysWorld* world, Vector3 origin, float radius,
    Vector3 direction, float maxDist,
    uint32_t layerMask, CollisionInfo* outHit); // Sweep a sphere along direction and return the first hit

/* ── Broadphase Algorithms ──────────────────────────────────────── */

void PHYS_WORLD_TestPairwise(PhysWorld* world, CollisionPairFn fn);       // Brute-force O(n²) pairwise collision test — fires fn for every overlapping pair
void PHYS_WORLD_TestSweepAndPrune(PhysWorld* world, CollisionPairFn fn);  // Sort boxes by min.x then sweep — O(n log n + k) for box-box; box-sphere and sphere-sphere remain brute-force

/* void PHYS_WORLD_SetLayerCollision(PhysWorld* world,
                                     int layerA, int layerB, bool collides); */
