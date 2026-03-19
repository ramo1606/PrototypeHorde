#pragma once

#include "rmem.h"
#include <stddef.h>

typedef MemPool MemoryArena;

/* Convenience macros */
#define ARENA_ALLOC(arena, type) \
    ((type *)MemPoolAlloc((arena), sizeof(type)))

#define ARENA_ALLOC_ARRAY(arena, type, count) \
    ((type *)MemPoolAlloc((arena), sizeof(type) * (count)))

/* Lifecycle helpers */
static inline MemoryArena ARENA_Create(size_t capacity) { return CreateMemPool(capacity); }
static inline void ARENA_Destroy(MemoryArena* arena) { DestroyMemPool(arena); }
static inline void ARENA_Reset(MemoryArena* arena) { MemPoolReset(arena); }
static inline void* ARENA_Alloc(MemoryArena* arena, size_t size) { return MemPoolAlloc(arena, size); }
static inline void ARENA_Free(MemoryArena* arena, void* ptr) { MemPoolFree(arena, ptr); }
static inline size_t ARENA_GetFreeMemory(MemoryArena arena) { return GetMemPoolFreeMemory(arena); }