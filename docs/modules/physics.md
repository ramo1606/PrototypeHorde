# physics (kit)

Collider pool with AABB / sphere / capsule primitives, layer-mask
filtering, iterative move-and-collide, raycast and overlap queries,
plus trigger detection. No rotations (AABBs always axis-aligned,
capsules always vertical), no continuous detection, no joints.

Lives under `lib/` as part of the reusable kit.

## Dependencies & overrides

```
DEPENDENCIES: raylib.h
OVERRIDES (define before include):
  MAX_COLLIDERS               (default 256)
  MOVE_AND_COLLIDE_ITERATIONS (default 3)
```

## Concepts

- **Collider** — shape + position + layer + mask + owner id.
- **Layer / Mask** — both `int` bitfields. `layer` is what the collider
  IS (single bit); `mask` is what it COLLIDES WITH (one or more bits).
  Two colliders test only when
  `(A.mask & B.layer) && (B.mask & A.layer)`.
- **Dynamic vs static** — only dynamic colliders move via
  `PhysicsMoveAndCollide`.
- **Trigger** — detects overlap but does not push out.

The module does **not** name any layers — define your own (e.g.
`#define LAYER_PLAYER (1 << 0)`) in your project. Prototype Horde's
set lives in `include/layers.h`.

See also: *Real-Time Collision Detection* (Christer Ericson) in
[`REFERENCES.md`](../../REFERENCES.md) for the geometric tests this
module implements.

## Types

- `ColliderHandle` — opaque integer index. `COLLIDER_HANDLE_INVALID = -1`.
- `ColliderType` — `AABB`, `SPHERE`, `CAPSULE`.
- `ColliderShape` — union over `ColliderAABB`, `ColliderSphere`,
  `ColliderCapsule`.
- `Collider` — record: type + shape + position + `int layer, mask` +
  ownerID + active/dynamic/trigger flags.
- `CollisionInfo` — raycast result.
- `ContactInfo` — body-body overlap; normal points **from B toward A**.
- `PhysWorld` — pool + per-frame stats.

## Public API

### Lifecycle / registration

| Function | Purpose |
|---|---|
| `PhysicsInit(*w)` | Zero the pool. |
| `PhysicsShutdown(*w)` | No-op. |
| `PhysicsAddBox(*w, pos, halfExtents, layer, mask, owner, dynamic, trigger)` | Add an AABB. |
| `PhysicsAddSphere(...)` | Add a sphere. |
| `PhysicsAddCapsule(...)` | Add a vertical capsule. |
| `PhysicsRemove(*w, handle)` | Free a slot. |

### Position / movement

| Function | Purpose |
|---|---|
| `PhysicsSetPosition(*w, h, pos)` | Teleport a collider. |
| `PhysicsGetPosition(*w, h)` | Read current position. |
| `PhysicsMoveAndCollide(*w, h, delta, layerMask, *grounded)` | Move dynamic by `delta`, resolve iteratively against `layerMask`, return final position. `grounded` (optional) is set if any contact had a mostly-upward normal. |

### Per-tick / queries

| Function | Purpose |
|---|---|
| `PhysicsUpdate(*w, **outContacts, *outCount)` | Process passive interactions: trigger overlaps and dynamic-vs-dynamic pairs not handled by `MoveAndCollide`. |
| `PhysicsRayCast(*w, origin, dir, maxDist, mask, *outHit)` | Closest hit matching the mask. |
| `PhysicsRayCastIgnore(*w, origin, dir, maxDist, mask, ignore, *outHit)` | Same, ignoring one collider (the shooter). |
| `PhysicsOverlapSphere(*w, center, radius, mask, outHandles[], maxResults)` | Returns count written. AOE damage. |

### Pure tests

`TestSphereVsAABB`, `TestSphereVsCapsule`, `TestCapsuleVsAABB`,
`TestCapsuleVsCapsule`, `TestRayVsAABB`, `TestRayVsCapsule`. Stateless.

### Debug

`PhysicsDebugDraw(*w)` — wireframes inside `BeginMode3D`. Green = static,
yellow = dynamic, red = trigger.

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

3 iterations escapes simple wedges. Order-dependent in pinch cases —
accepted, the world is simple boxes.

## Layer-mask test

Two colliders interact only when **both** filters allow it:

```c
bool pairAllowed(A, B) {
    return (A.mask & B.layer) && (B.mask & A.layer);
}
```

Lets one side opt out asymmetrically (a projectile that ignores its
shooter, a trigger that watches enemies but not pickups) without
changing the other side's mask.

## Pure-vs-state separation

The `Test*` functions are stateless geometry. The `Physics*` functions
wrap them with iteration over the pool, mask filtering, and result
aggregation. Keeps unit-testing easy and lets gameplay reuse a single
test for ad-hoc cases (e.g. testing a candidate position against one
specific AABB without registering it).

## Notes

- AABBs always axis-aligned; capsules always vertical (Y-up).
- Contact normal convention: **from B toward A**. Stay consistent or
  `MoveAndCollide` pushes the wrong way.
- The static `s_triggerContacts` buffer caps trigger throughput per
  tick; ample for the current scope.
