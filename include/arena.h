#pragma once

/*
 * arena.h — Memory arena (bump allocator with freelist).
 *
 * DEPENDENCIES: rmem.h
 * OVERRIDES: none
 *
 * Header-only wrapper over rmem's MemPool. Provides bump-allocator
 * arenas with freelist support. Pass arenas by value when querying,
 * by pointer when mutating.
 */

#include "rmem.h"
#include <stddef.h>

/* MemArena (not "Arena", to avoid collision with rmem's internal Arena type). */
typedef MemPool MemArena;

static inline MemArena ArenaCreate(size_t capacity)            { return CreateMemPool(capacity); }
static inline void     ArenaDestroy(MemArena* arena)           { DestroyMemPool(arena); }
static inline void     ArenaReset(MemArena* arena)             { MemPoolReset(arena); }

static inline void*    ArenaAlloc(MemArena* arena, size_t size){ return MemPoolAlloc(arena, size); }
static inline void     ArenaFree(MemArena* arena, void* ptr)   { MemPoolFree(arena, ptr); }

static inline size_t   ArenaGetFreeMemory(MemArena arena)      { return GetMemPoolFreeMemory(arena); }

#define ARENA_ALLOC(arena, type) \
    ((type*)ArenaAlloc((arena), sizeof(type)))

#define ARENA_ALLOC_ARRAY(arena, type, count) \
    ((type*)ArenaAlloc((arena), sizeof(type) * (count)))
