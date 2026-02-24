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

#define PHYS_WORLD_MAX_BOXES   512
#define PHYS_WORLD_MAX_SPHERES 512
#define PHYS_WORLD_MAX_CAPSULES 256                 /* Phase 5 */

struct CollisionInfo
{
    bool hit;
    float distance;
    Vector3 point;
    Vector3 normal;
    Component* collider;
    Actor* actor;
};

typedef void (*CollisionPairFn)(Actor* a, Actor* b, Vector3 normal, float penetration);

/* Phase 5: detailed collision callback with full contact info */
struct ContactInfo
{
    Actor* actorA;
    Actor* actorB;
    Component* colliderA;
    Component* colliderB;
    Vector3 contactPoint;
    Vector3 contactNormal;
    float   penetration;
};

typedef void (*ContactCallbackFn)(const ContactInfo* contact);

struct PhysWorld
{
    BoxComponent* boxes[PHYS_WORLD_MAX_BOXES];
    int boxCount;
    SphereComponent* spheres[PHYS_WORLD_MAX_SPHERES];
    int sphereCount;

    /* CapsuleComponent* capsules[PHYS_WORLD_MAX_CAPSULES]; */  /* Phase 5 */
    /* int capsuleCount; */

    /* Phase 5: collision layer matrix */
    /* uint32_t layerMatrix[32]; */

    /* Phase 5: spatial grid */
    /* SpatialGrid* grid; */

    /* Callbacks */
    ContactCallbackFn onContact;            /* Called per contact */
    CollisionPairFn onPairCollision;      /* Legacy pair callback */
};

void PHYS_WORLD_Init(PhysWorld* world);
void PHYS_WORLD_Shutdown(PhysWorld* world);
void PHYS_WORLD_Update(PhysWorld* world, float deltaTime);

void PHYS_WORLD_AddBox(PhysWorld* world, BoxComponent* box);
void PHYS_WORLD_RemoveBox(PhysWorld* world, BoxComponent* box);
void PHYS_WORLD_AddSphere(PhysWorld* world, SphereComponent* sphere);
void PHYS_WORLD_RemoveSphere(PhysWorld* world, SphereComponent* sphere);
/* void PHYS_WORLD_AddCapsule(PhysWorld* world, CapsuleComponent* cap); */
/* void PHYS_WORLD_RemoveCapsule(PhysWorld* world, CapsuleComponent* cap); */

bool PHYS_WORLD_RayCast(PhysWorld* world, Ray ray, float maxDist, uint32_t layerMask, 
    CollisionInfo* outHit);
bool PHYS_WORLD_RayCastIgnore(PhysWorld* world, Ray ray, float maxDist, uint32_t layerMask, 
    Actor* ignore, CollisionInfo* outHit);

int PHYS_WORLD_OverlapSphere(PhysWorld* world, Vector3 center, float radius, 
    uint32_t layerMask, Actor** outActors, int maxResults);
int PHYS_WORLD_OverlapBox(PhysWorld* world, BoundingBox box, 
    uint32_t layerMask, Actor** outActors, int maxResults);

bool PHYS_WORLD_SphereCast(PhysWorld* world, Vector3 origin, float radius, 
    Vector3 direction, float maxDist, 
    uint32_t layerMask, CollisionInfo* outHit);

void PHYS_WORLD_TestPairwise(PhysWorld* world, CollisionPairFn fn);
void PHYS_WORLD_TestSweepAndPrune(PhysWorld* world, CollisionPairFn fn);

/* void PHYS_WORLD_SetLayerCollision(PhysWorld* world,
                                     int layerA, int layerB, bool collides); */