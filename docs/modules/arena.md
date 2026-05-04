# arena (kit)

Header-only wrapper over rmem's `MemPool`. Provides memory arenas (bump
allocators with freelist support).

Lives under `lib/` as part of the reusable kit.

## Dependencies & overrides

```
DEPENDENCIES: rmem.h
OVERRIDES: none
```

The implementation is `static inline` inside `arena.h`. There is no
`arena.c`. Drop the header anywhere; it pulls in `rmem.h` and inlines.

## Types

- `MemArena` — alias of `MemPool`. Lightweight struct: pointer to the
  block, size, current offset, freelist. Cheap to pass by value.
  Named `MemArena` (not `Arena`) to avoid collision with rmem's
  internal `Arena` type.

## Public API

| Function | Purpose |
|---|---|
| `ArenaCreate(capacity)` | Allocate a contiguous block of `capacity` bytes. |
| `ArenaDestroy(*a)` | Free the entire block. |
| `ArenaReset(*a)` | Move the bump pointer back to zero. Invalidates all prior allocations. |
| `ArenaAlloc(*a, size)` | Allocate aligned bytes. Returns NULL if out of space. |
| `ArenaFree(*a, ptr)` | Free a single pointer (rmem keeps a freelist). Specific patterns only. |
| `ArenaGetFreeMemory(a)` | Bytes remaining. Debug overlay only. |

## Macros

- `ARENA_ALLOC(arena, type)` — allocates one object of the given type and casts.
- `ARENA_ALLOC_ARRAY(arena, type, count)` — allocates an array of `count` elements.

## Typical lifetimes

Three arenas, embedded by value in the host's main struct:

| Arena | Use | When it resets |
|---|---|---|
| `permanent` | Definition tables, config | Never |
| `level` | Per-level state, entity pools | Level transition |
| `scratch` | Per-frame temporaries | Every visual frame |

## Notes

- **Pass by value vs by pointer.** `ArenaGetFreeMemory` takes
  `MemArena` by value (rmem's internal query requires it). The rest
  take `MemArena*` because they mutate state.
- **Reset doesn't destroy contents.** If a struct allocated in the
  arena holds external resources (raylib models with GPU resources,
  file handles), release them explicitly **before** the reset. The
  arena only manages the RAM block.
- **Allocation failure.** `ArenaAlloc` returns NULL on OOM. No caller
  in this project checks; the assumption is that configured sizes are
  sufficient. If it fails, increase the size; do not add a heap fallback.
