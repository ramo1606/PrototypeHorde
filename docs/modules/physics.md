# physics

Simple physics world with AABB / sphere / capsule primitives, layer-mask
filtering, iterative move-and-collide, raycast and overlap queries, plus
trigger detection. No rotations (AABBs are always axis-aligned, capsules
always vertical), no continuous detection, no joints. Designed for an
isometric arena with flat ground, box obstacles, and upright character
capsules.

## Concepts

- **Collider** — a shape (AABB / sphere / capsule), a world position, a
  layer, a mask, and an owner id (gameplay-defined index, `-1` for
  scenery).
- **Layer / Mask** — both bitfields. `layer` is what the collider IS
  (single bit); `mask` is what it COLLIDES WITH (one or more bits).
  Two colliders test only when `(A.mask & B.layer) && (B.mask & A.layer)`.
- **Dynamic vs static** — only dynamic colliders move via
  `PhysicsMoveAndCollide`. Static colliders are immovable scenery.
- **Trigger** — detects overlap but does not push out. Used for pickups,
  mines, zone volumes.

## Types

- `ColliderHandle` — opaque integer index. `COLLIDER_HANDLE_INVALID = -1`.
- `CollisionLayer` — bitflag enum (`PLAYER`, `ENEMY`, `PROJECTILE`,
  `SCENERY`, `PICKUP`, `TRIGGER`, `DEPLOYABLE`, plus `MASK_ALL` /
  `MASK_NONE`).
- `ColliderType` — `AABB`, `SPHERE`, `CAPSULE`.
- `ColliderShape` — union over the three shape structs.
- `Collider` — full record.
- `CollisionInfo` — raycast result (`hit`, `distance`, `point`, `normal`,
  `collider`, `ownerID`).
- `ContactInfo` — body-body overlap result (`hit`, `penetration`, `normal`,
  `contactPoint`, `colliderA/B`, `ownerA/B`). Normal points
  **from B toward A**; pushing A by `normal * penetration` resolves.
- `PhysWorld` — owns the collider pool and per-frame stats.

## Public API

### Lifecycle

| Function | Purpose |
|---|---|
| `PhysicsInit(*w)` | Zero the pool. |
| `PhysicsShutdown(*w)` | Currently a no-op; reserved for future cleanup. |

### Registration

| Function | Purpose |
|---|---|
| `PhysicsAddBox(*w, pos, halfExtents, layer, mask, owner, dynamic, trigger)` | Add an AABB. |
| `PhysicsAddSphere(*w, pos, radius, layer, mask, owner, dynamic, trigger)` | Add a sphere. |
| `PhysicsAddCapsule(*w, pos, radius, halfHeight, layer, mask, owner, dynamic, trigger)` | Add a vertical capsule. |
| `PhysicsRemove(*w, handle)` | Free a slot. |

All `Add*` calls return a `ColliderHandle` or `COLLIDER_HANDLE_INVALID`
if the pool is full (`MAX_COLLIDERS` from `config.h`).

### Position

| Function | Purpose |
|---|---|
| `PhysicsSetPosition(*w, handle, pos)` | Teleport a collider. |
| `PhysicsGetPosition(*w, handle)` | Read current world position. |

### Movement

| Function | Purpose |
|---|---|
| `PhysicsMoveAndCollide(*w, handle, delta, layerMask, *grounded)` | Move dynamic by `delta`, resolve iteratively against `layerMask`, return final position. `grounded` (optional) is set true if any contact had a mostly-upward normal. |

### Per-tick

| Function | Purpose |
|---|---|
| `PhysicsUpdate(*w, **outContacts, *outCount)` | Process passive interactions: trigger overlaps and any dynamic-vs-dynamic pairs not handled by `MoveAndCollide`. Trigger contacts are written to a `ContactInfo` array (currently a static buffer; see Task 2.7 for the planned move to scratch arena). Pass `NULL` for both outs if you don't need them. |

### Queries

| Function | Purpose |
|---|---|
| `PhysicsRayCast(*w, origin, dir, maxDist, layerMask, *outHit)` | Closest hit matching the mask. |
| `PhysicsRayCastIgnore(*w, origin, dir, maxDist, layerMask, ignore, *outHit)` | Same, ignoring one specific collider (the shooter). |
| `PhysicsOverlapSphere(*w, center, radius, layerMask, outHandles[], maxResults)` | Returns count written. Used for AOE damage. |

### Pure tests

`TestSphereVsAABB`, `TestSphereVsCapsule`, `TestCapsuleVsAABB`,
`TestCapsuleVsCapsule`, `TestRayVsAABB`, `TestRayVsCapsule`. Stateless
geometry tests; gameplay can use them directly for ad-hoc cases.

### Debug

`PhysicsDebugDraw(*w)` — wireframes inside `BeginMode3D`. Green = static,
yellow = dynamic, red = trigger. Registered into the debug overlay slot 2
(F4) by `game.c`.

## MoveAndCollide algorithm

```
pos += delta
for iter in 0..MOVE_AND_COLLIDE_ITERATIONS:
    contacts = collect overlapping pairs (mover vs world, masked)
    if no contacts: break
    for each contact:
        pos += contact.normal * contact.penetration
    if grounded out param: set grounded if any normal.y > 0.7
```

3 iterations is enough to escape simple wedges. Order-dependent in pinch
cases — accepted, the world geometry is simple boxes.

## Layer-mask test

Two colliders interact only when **both** filters allow it:

```c
bool pairAllowed(A, B) {
    return (A.mask & B.layer) && (B.mask & A.layer);
}
```

This lets one side opt out asymmetrically (a projectile that ignores its
shooter, a trigger that watches enemies but not pickups) without changing
the other side's mask.

## Pure-vs-state separation

The `Test*` functions are stateless, take primitives, return contact
info. The `PhysWorld` functions wrap them with iteration over the pool,
mask filtering, and result aggregation. Keeping the geometry pure keeps
unit-testing easy and lets gameplay reuse a single test (e.g. testing
the camera against one specific AABB without registering it).

## Pending improvements (Task 2.7)

- **Generation handles.** `(generation << 16) | index` to detect
  use-after-free when a slot is reused. Today, holding an old handle into
  a recycled slot silently aliases the new occupant.
- **Compact active list.** Today `MoveAndCollide` and `Update` walk the
  whole `MAX_COLLIDERS` array. With a compact active-index list, work is
  proportional to live colliders instead of pool capacity.
- **Trigger buffer in scratch arena.** The static `s_triggerContacts`
  buffer caps trigger throughput per tick. Moving it to the scratch arena
  removes the fixed cap and matches the rest of the per-frame allocation
  pattern.

## Notes

- AABBs are always axis-aligned; capsules are always vertical (Y-up).
  Tilted shapes would require an orientation field and complicate every
  test.
- The contact normal convention (B→A) is the same across all `Test*`
  functions. Stay consistent or `MoveAndCollide` will push the wrong way.
- `MAX_COLLIDERS` lives in `config.h`. It is allocated as a fixed-size
  array inside `PhysWorld` (lives by value in `Game`).
