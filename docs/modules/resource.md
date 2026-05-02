# resource

Centralized loader for game assets (models, textures, shaders, sounds, fonts).
Resources are identified by an enum (`ResourceID`) and looked up directly by
index in fixed-size arrays — no string lookups, no hashing.

## Types

- `ResourceID` — enum listing every asset in the game. `RES_COUNT` is the
  array size. Adding an asset means adding an enum entry **and** a row in the
  internal `resourceTable[]`.
- `ResourceType` — discriminator: model / texture / shader / sound / font.
  Selects which raylib `Load*`/`Unload*` calls to use.

## Public API

| Function | Purpose |
|---|---|
| `ResourceInit()` | Clear the loaded-flags table. Call once at startup. |
| `ResourceShutdown()` | Unload everything still loaded. Call once at exit. |
| `ResourceLoad(id)` | Load a single asset. Idempotent — no-op if already loaded. |
| `ResourceUnload(id)` | Unload a single asset. No-op if not loaded. |
| `ResourceLoadGroup(ids, count)` | Load a batch in one call. |
| `ResourceUnloadGroup(ids, count)` | Unload a batch in one call. |
| `ResourceGetModel(id)` | Get a loaded model by id. Asserts on miss. |
| `ResourceGetTexture(id)` | Get a loaded texture by id. Asserts on miss. |
| `ResourceGetShader(id)` | Get a loaded shader by id. Asserts on miss. |
| `ResourceGetSound(id)` | Get a loaded sound by id. Asserts on miss. |
| `ResourceGetFont(id)` | Get a loaded font by id. Asserts on miss. |
| `ResourceIsLoaded(id)` | Check whether a given asset is loaded. |

## Storage

One static array per type, sized `RES_COUNT`. The `ResourceID` is the index.
Slots for ids whose type does not match the array (e.g. a model id in
`textures[]`) are simply unused. This wastes a few struct slots, but keeps
lookup O(1) without a level of indirection.

A separate `loaded[RES_COUNT]` boolean array tracks which ids are populated.

## Lifetime patterns

- **Permanent assets** (UI font, common shaders): loaded in `ResourceInit`-
  adjacent code, never unloaded until shutdown.
- **Per-level assets** (level-specific models, sounds): loaded by the level's
  `Enter` callback via `ResourceLoadGroup`, unloaded in `Exit` via
  `ResourceUnloadGroup`. Combined with the level memory arena reset, this
  bounds memory usage between levels.

## Technical notes

### Why direct array indexing instead of a hash map

`RES_COUNT` is small (under a hundred even at full game scope), so `O(1)`
direct indexing beats any hash for both speed and simplicity. No collisions,
no resizing, no allocation at load time.

### Internal `findEntry` is linear

`findEntry` walks `resourceTable[]` to find the row matching an `id`. It is
O(n) but only runs once per load/unload, never per frame. Not worth a second
index table.

### Shader path convention

For `RESTYPE_SHADER`, the path stored in the table is the **base name**
without extension. The loader appends `.vs` and `.fs` automatically. Both
files must exist.

### Asserts on `Get*`

Calling `ResourceGetModel(id)` on an unloaded id triggers an assertion. There
is no fallback or null model. The contract is: load before you get. This
catches missing-load bugs immediately during development.
