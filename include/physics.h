/**
 * physics.h — Physics World: Colliders, Detection, and Queries
 *
 * A simple physics system with AABB, sphere, and capsule primitives.
 * No rotation (AABBs are always axis-aligned), no continuous detection,
 * no joints. Designed for a top-down-ish TPS where the arena is flat
 * with box obstacles and characters are vertical capsules.
 *
 * Key concepts:
 *
 *   Collider    — A shape in the world (AABB, sphere, or capsule) with
 *                 a position, layer, mask, and owner ID.
 *
 *   Layer/Mask  — Bitflag system. A collider's `layer` says what it IS,
 *                 its `mask` says what it collides WITH. Two colliders
 *                 only test if (A.mask & B.layer) && (B.mask & A.layer).
 *
 *   Dynamic     — A dynamic collider can be moved with MoveAndCollide.
 *                 Static colliders are immovable obstacles.
 *
 *   CollisionInfo — Result of a raycast: hit point, normal, distance.
 *   ContactInfo   — Result of a body-body test: penetration depth, normal,
 *                   contact point, and references to both colliders.
 *
 * Usage patterns:
 *
 *   Registration:  handle = PHYS_WORLD_AddBox(world, ...)
 *   Movement:      finalPos = PHYS_WORLD_MoveAndCollide(world, handle, delta, mask)
 *   Queries:       PHYS_WORLD_RayCast(world, origin, dir, maxDist, mask, &hit)
 *   Each tick:     PHYS_WORLD_Update(world) — processes triggers, passive pairs
 *   Debug:         PHYS_WORLD_DebugDraw(world) — wireframe visualization
 */

#pragma once

#include "raylib.h"
#include "raymath.h"
#include "config.h"
#include <stdbool.h>

typedef struct PhysWorld PhysWorld;
typedef struct Collider Collider;

/* ── Handle ──────────────────────────────────────────────────────────────── */

typedef int ColliderHandle;
#define COLLIDER_HANDLE_INVALID (-1)

/* ── Collision Layers (bitflags) ─────────────────────────────────────────── */

/*
 * Each layer is a single bit. A collider's `layer` field has exactly
 * one bit set (what it IS). Its `mask` field can have multiple bits
 * (what it collides WITH).
 *
 * Example: a player projectile
 *   layer = COLLISION_LAYER_PROJECTILE
 *   mask  = COLLISION_LAYER_ENEMY | COLLISION_LAYER_SCENERY
 *   → hits enemies and walls, ignores player, other projectiles, pickups
 */
typedef enum CollisionLayer
{
    COLLISION_LAYER_PLAYER = (1 << 0),
    COLLISION_LAYER_ENEMY = (1 << 1),
    COLLISION_LAYER_PROJECTILE = (1 << 2),
    COLLISION_LAYER_SCENERY = (1 << 3),
    COLLISION_LAYER_PICKUP = (1 << 4),
    COLLISION_LAYER_TRIGGER = (1 << 5),
    COLLISION_LAYER_DEPLOYABLE = (1 << 6),

    /* Convenience masks */
    COLLISION_MASK_ALL = 0x7F,
    COLLISION_MASK_NONE = 0,
} CollisionLayer;

/* ── Collider Types ──────────────────────────────────────────────────────── */

typedef enum ColliderType
{
    COLLIDER_AABB,
    COLLIDER_SPHERE,
    COLLIDER_CAPSULE,
} ColliderType;

/* ── Shape Data ──────────────────────────────────────────────────────────── */

/*
 * All shapes are defined relative to the collider's position.
 * The position is the CENTER of the shape in world space.
 */

 /* Axis-Aligned Bounding Box: position ± halfExtents on each axis. */
typedef struct ColliderAABB
{
    Vector3 halfExtents;        /* Half-size on each axis                   */
} ColliderAABB;

/* Sphere: position is center, radius defines size. */
typedef struct ColliderSphere
{
    float radius;
} ColliderSphere;

/*
 * Capsule: a cylinder with hemispherical caps, oriented along Y (vertical).
 * The position is the CENTER of the capsule (midpoint between the two caps).
 * Total height = 2 * halfHeight + 2 * radius.
 *
 * The "line segment" of the capsule goes from:
 *   bottom = position.y - halfHeight
 *   top    = position.y + halfHeight
 * And the radius extends from every point on that segment.
 *
 * Why vertical only? All characters and enemies in this game stand upright.
 * Tilted capsules would require storing an orientation and complicate every
 * test. We don't need that complexity.
 */
typedef struct ColliderCapsule
{
    float radius;
    float halfHeight;           /* Half of the inner line segment length    */
} ColliderCapsule;

/* ── Shape Union ─────────────────────────────────────────────────────────── */

typedef union ColliderShape
{
    ColliderAABB box;
    ColliderSphere sphere;
    ColliderCapsule capsule;
} ColliderShape;

/* ── Collider ────────────────────────────────────────────────────────────── */

struct Collider
{
    ColliderType type;
    ColliderShape shape;

    Vector3 position;           /* World-space center                       */

    CollisionLayer layer;       /* What this collider IS (single bit)       */
    CollisionLayer mask;        /* What this collider collides WITH (bits)  */

    int ownerID;                /* Gameplay entity that owns this collider.
                                 * Convention: positive = entity index,
                                 * -1 = no owner (scenery). Gameplay
                                 * interprets this, physics doesn't care.  */

    bool active;                /* Slot in use?                             */
    bool dynamic;               /* Can be moved with MoveAndCollide?        */
    bool trigger;               /* If true, detects overlap but doesn't
                                 * resolve (no push-out). Used for pickups,
                                 * mines, zone triggers.                    */
};

/* ── Collision Results ───────────────────────────────────────────────────── */

/*
 * CollisionInfo: result of a raycast or point query.
 * "I cast a ray and it hit something — where, how far, what?"
 */
typedef struct CollisionInfo
{
    bool hit;                   /* Did the ray hit anything?                */
    float distance;             /* Distance from ray origin to hit point   */
    Vector3 point;              /* World-space hit point                    */
    Vector3 normal;             /* Surface normal at hit point             */
    ColliderHandle collider;    /* Handle of the collider that was hit     */
    int ownerID;                /* Owner of the hit collider               */
} CollisionInfo;

/*
 * ContactInfo: result of a body-body overlap test.
 * "These two colliders are overlapping — how much, in what direction?"
 */
typedef struct ContactInfo
{
    bool hit;                   /* Are the two shapes overlapping?          */
    float penetration;          /* How deep the overlap is (positive)      */
    Vector3 normal;             /* Direction to push A away from B         */
    Vector3 contactPoint;       /* Approximate contact point (world space) */

    ColliderHandle colliderA;   /* First collider in the pair              */
    ColliderHandle colliderB;   /* Second collider in the pair             */
    int ownerA;                 /* Owner of collider A                     */
    int ownerB;                 /* Owner of collider B                     */
} ContactInfo;

/* ── Physics World ───────────────────────────────────────────────────────── */

/*
 * The PhysWorld owns all colliders and provides detection/query services.
 * It lives by value inside the Game struct, like the Renderer.
 */
struct PhysWorld
{
    Collider colliders[MAX_COLLIDERS];
    int colliderCount;          /* Number of active colliders               */

    /* Per-frame stats (for debug overlay) */
    int statsPairsChecked;      /* Pairs tested this tick                   */
    int statsContactsFound;     /* Contacts detected this tick              */
    int statsTriggersFound;     /* Trigger overlaps detected this tick      */
};

/* ── Lifecycle ───────────────────────────────────────────────────────────── */

void PHYS_WORLD_Init(PhysWorld* world);
void PHYS_WORLD_Shutdown(PhysWorld* world);

/* ── Registration ────────────────────────────────────────────────────────── */

/*
 * Add a collider to the world. Returns a handle for future reference.
 * The `dynamic` flag determines if MoveAndCollide can move it.
 * The `trigger` flag makes it detect overlaps without resolving.
 */
ColliderHandle PHYS_WORLD_AddBox(PhysWorld* world, Vector3 position,
    Vector3 halfExtents, int layer, int mask, int ownerID,
    bool dynamic, bool trigger);

ColliderHandle PHYS_WORLD_AddSphere(PhysWorld* world, Vector3 position,
    float radius, int layer, int mask, int ownerID,
    bool dynamic, bool trigger);

ColliderHandle PHYS_WORLD_AddCapsule(PhysWorld* world, Vector3 position,
    float radius, float halfHeight, int layer, int mask, int ownerID,
    bool dynamic, bool trigger);

void PHYS_WORLD_Remove(PhysWorld* world, ColliderHandle handle);

/* ── Transform Updates ───────────────────────────────────────────────────── */

/* Directly set the world-space position of a collider. */
void PHYS_WORLD_SetPosition(PhysWorld* world, ColliderHandle handle,
    Vector3 position);

/* Get the current world-space position of a collider. */
Vector3 PHYS_WORLD_GetPosition(const PhysWorld* world, ColliderHandle handle);

/* ── Movement with Collision Resolution ──────────────────────────────────── */

/*
 * Move a dynamic collider by `delta`, resolving collisions iteratively.
 * Returns the final world-space position after resolution.
 *
 * Internally:
 *   1. Apply delta to position
 *   2. Test against all colliders matching layerMask
 *   3. If penetrating, push out along contact normal
 *   4. Repeat for up to MOVE_AND_COLLIDE_ITERATIONS
 *
 * This is the primary movement function for gameplay entities (player,
 * enemies). They call this from their own Update, giving explicit
 * control over movement order.
 *
 * The `grounded` output (optional, can be NULL) is set to true if any
 * resolved contact had a mostly-upward normal (floor contact). Useful
 * for jump logic.
 */
#define MOVE_AND_COLLIDE_ITERATIONS 3

Vector3 PHYS_WORLD_MoveAndCollide(PhysWorld* world, ColliderHandle handle,
    Vector3 delta, int layerMask, bool* grounded);

/* ── Per-Tick Update ─────────────────────────────────────────────────────── */

/*
 * Process passive interactions: trigger overlaps, dynamic-vs-dynamic
 * pairs that weren't moved with MoveAndCollide this tick.
 *
 * Trigger callbacks are stored in a ContactInfo array in the scratch arena.
 * Gameplay systems read this array to react (pickup collected, mine
 * triggered, zone entered).
 *
 * `outContacts` receives the array pointer, `outCount` the number of
 * contacts. Both can be NULL if the caller doesn't need trigger info.
 */
void PHYS_WORLD_Update(PhysWorld* world,
    ContactInfo** outContacts, int* outCount);

/* ── Queries ─────────────────────────────────────────────────────────────── */

/*
 * Cast a ray and return the closest hit matching the layer mask.
 * Returns true if anything was hit.
 */
bool PHYS_WORLD_RayCast(const PhysWorld* world, Vector3 origin, Vector3 dir,
    float maxDist, int layerMask, CollisionInfo* outHit);

/*
 * Same as RayCast but ignores one specific collider handle.
 * Used for shooting: the player's own capsule should not block their shots.
 */
bool PHYS_WORLD_RayCastIgnore(const PhysWorld* world, Vector3 origin,
    Vector3 dir, float maxDist, int layerMask,
    ColliderHandle ignore, CollisionInfo* outHit);

/*
 * Find all colliders whose shapes overlap a sphere.
 * Returns the number of results written to `outHandles` (up to maxResults).
 * Used for AOE damage (explosions, grenades).
 */
int PHYS_WORLD_OverlapSphere(const PhysWorld* world, Vector3 center,
    float radius, int layerMask,
    ColliderHandle* outHandles, int maxResults);

/* ── Collision Tests (pure functions, no state) ──────────────────────────── */

/*
 * These test pairs of shapes and return contact information.
 * They don't read or modify the PhysWorld — they're pure geometry.
 * Exposed publicly so gameplay can use them for ad-hoc tests
 * (e.g. camera collision against a single AABB).
 *
 * Convention: the contact normal points FROM shape B TOWARD shape A.
 * Pushing A along normal * penetration resolves the overlap.
 */

bool TestSphereVsAABB(Vector3 sphereCenter, float sphereRadius,
    Vector3 boxCenter, Vector3 boxHalfExtents,
    ContactInfo* outContact);

bool TestSphereVsCapsule(Vector3 sphereCenter, float sphereRadius,
    Vector3 capsuleCenter, float capsuleRadius, float capsuleHalfHeight,
    ContactInfo* outContact);

bool TestCapsuleVsAABB(Vector3 capsuleCenter, float capsuleRadius,
    float capsuleHalfHeight,
    Vector3 boxCenter, Vector3 boxHalfExtents,
    ContactInfo* outContact);

bool TestCapsuleVsCapsule(Vector3 centerA, float radiusA, float halfHeightA,
    Vector3 centerB, float radiusB, float halfHeightB,
    ContactInfo* outContact);

bool TestRayVsAABB(Vector3 origin, Vector3 dir, float maxDist,
    Vector3 boxCenter, Vector3 boxHalfExtents,
    CollisionInfo* outHit);

bool TestRayVsCapsule(Vector3 origin, Vector3 dir, float maxDist,
    Vector3 capsuleCenter, float capsuleRadius, float capsuleHalfHeight,
    CollisionInfo* outHit);

/* ── Debug Drawing ───────────────────────────────────────────────────────── */

/*
 * Draw wireframes for all active colliders.
 * Call INSIDE BeginMode3D (registered via DEBUG_Register3D).
 *
 * Colors:
 *   Green  = static
 *   Yellow = dynamic
 *   Red    = trigger
 */
void PHYS_WORLD_DebugDraw(const PhysWorld* world);