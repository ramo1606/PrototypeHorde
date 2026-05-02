# arena

Wrapper over rmem's `MemPool`. Provides memory arenas (bump allocators with
freelist support) for the three lifetimes of the game: permanent, level, scratch.

## Types

- `MemArena` — direct alias of rmem's `MemPool`. Lightweight struct: holds a
  pointer to the block, total size, current offset, and freelist. Cheap to
  pass by value.

  The type is named `MemArena` and not `Arena` because rmem internally defines
  its own `Arena` type. The function API still uses the `Arena*` prefix.

## Public API

| Function | Purpose |
|---|---|
| `ArenaCreate(capacity)` | Create an arena with a contiguous block of `capacity` bytes. |
| `ArenaDestroy(*a)` | Free the entire block. Do not use the arena after this. |
| `ArenaReset(*a)` | Reset the bump pointer. Invalidates all previous allocations. |
| `ArenaAlloc(*a, size)` | Allocate `size` aligned bytes. Returns `NULL` if there is no space. |
| `ArenaFree(*a, ptr)` | Free an individual pointer (rmem keeps a freelist). Useful only in specific patterns. |
| `ArenaGetFreeMemory(a)` | Bytes remaining. Debug overlay only. |

## Macros

- `ARENA_ALLOC(arena, type)` — allocates one object of the given type and casts.
- `ARENA_ALLOC_ARRAY(arena, type, count)` — allocates an array of `count` elements.

## Lifetimes in the project

Three arenas embedded by value in the `Game` struct:

| Arena | Size | Use | When it resets |
|---|---|---|---|
| `permanent` | 2 MB | Definition tables, config | Never |
| `level` | 8 MB | Entity pools, level geometry | Level transition |
| `scratch` | 1 MB | Per-frame temporaries (queries, lists) | Every frame |

## Technical notes

### Passing `Arena` by value vs by pointer

`ArenaGetFreeMemory` takes `Arena` by value (rmem's internal query requires
it). The rest take `Arena*` because they mutate state. The struct is small;
copying it is not a performance concern.

### `ArenaReset` does not destroy contents

`ArenaReset` only moves the bump pointer back to zero. It does not call
destructors or zero out the memory. If a struct allocated in the arena holds
external resources (raylib models with GPU resources, file handles), they
must be released explicitly **before** the reset. The arena only manages
the RAM block.

### Allocation failure

`ArenaAlloc` may return `NULL` if the arena is full. No caller checks today —
the assumption is that the sizes configured in `config.h` are sufficient.
If it ever fails, that's a capacity bug: increase the size in config, do not
add a heap fallback.
