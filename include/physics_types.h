#pragma once

#include "raylib.h"
#include "config.h"
#include <stdbool.h>

typedef int ColliderHandle;
#define COLLIDER_HANDLE_INVALID (-1)

#define MOVE_AND_COLLIDE_ITERATIONS 3

/* ── Collision Layers (bitflags) ─────────────────────────────────────────── */
/* A collider's `layer` says what it IS (one bit). Its `mask` says what
 * it collides WITH (one or more bits). Two colliders test only when
 * (A.mask & B.layer) && (B.mask & A.layer). */
typedef enum
{
    COLLISION_LAYER_PLAYER     = (1 << 0),
    COLLISION_LAYER_ENEMY      = (1 << 1),
    COLLISION_LAYER_PROJECTILE = (1 << 2),
    COLLISION_LAYER_SCENERY    = (1 << 3),
    COLLISION_LAYER_PICKUP     = (1 << 4),
    COLLISION_LAYER_TRIGGER    = (1 << 5),
    COLLISION_LAYER_DEPLOYABLE = (1 << 6),

    COLLISION_MASK_ALL  = 0x7F,
    COLLISION_MASK_NONE = 0,
} CollisionLayer;

typedef enum
{
    COLLIDER_AABB,
    COLLIDER_SPHERE,
    COLLIDER_CAPSULE,
} ColliderType;

/* ── Shape data ──────────────────────────────────────────────────────────── */
/* All shapes are centered on the collider's `position`. */

typedef struct ColliderAABB
{
    Vector3 halfExtents;
} ColliderAABB;

typedef struct ColliderSphere
{
    float radius;
} ColliderSphere;

/* Capsule: cylinder with hemispherical caps, oriented along Y. The line
 * segment runs from `position.y ± halfHeight`, with `radius` extending
 * from every point on it. Total height = 2*halfHeight + 2*radius. */
typedef struct ColliderCapsule
{
    float radius;
    float halfHeight;
} ColliderCapsule;

typedef union
{
    ColliderAABB    box;
    ColliderSphere  sphere;
    ColliderCapsule capsule;
} ColliderShape;

typedef struct Collider
{
    ColliderType   type;
    ColliderShape  shape;

    Vector3        position;     /* World-space center */

    CollisionLayer layer;        /* What this collider IS (single bit) */
    CollisionLayer mask;         /* What this collider hits (bits)     */

    int  ownerID;                /* Gameplay-defined; physics doesn't care.
                                  * Convention: -1 = scenery / no owner. */

    bool active;
    bool dynamic;                /* Movable via MoveAndCollide?         */
    bool trigger;                /* Detect overlap but don't resolve?   */
} Collider;

/* ── Collision results ───────────────────────────────────────────────────── */

/* Result of a raycast or point query. */
typedef struct CollisionInfo
{
    bool           hit;
    float          distance;
    Vector3        point;
    Vector3        normal;
    ColliderHandle collider;
    int            ownerID;
} CollisionInfo;

/* Result of a body-body overlap. Normal points FROM B TOWARD A; pushing
 * A along normal * penetration resolves the overlap. */
typedef struct ContactInfo
{
    bool           hit;
    float          penetration;
    Vector3        normal;
    Vector3        contactPoint;

    ColliderHandle colliderA;
    ColliderHandle colliderB;
    int            ownerA;
    int            ownerB;
} ContactInfo;

typedef struct PhysWorld
{
    Collider colliders[MAX_COLLIDERS];
    int      colliderCount;

    /* Per-frame stats */
    int statsPairsChecked;
    int statsContactsFound;
    int statsTriggersFound;
} PhysWorld;
