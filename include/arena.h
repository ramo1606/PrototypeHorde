#pragma once

#include "arena_types.h"
#include <stddef.h>

MemArena ArenaCreate(size_t capacity);
void     ArenaDestroy(MemArena* arena);
void     ArenaReset(MemArena* arena);

void*    ArenaAlloc(MemArena* arena, size_t size);
void     ArenaFree(MemArena* arena, void* ptr);

size_t   ArenaGetFreeMemory(MemArena arena);

#define ARENA_ALLOC(arena, type) \
    ((type*)ArenaAlloc((arena), sizeof(type)))

#define ARENA_ALLOC_ARRAY(arena, type, count) \
    ((type*)ArenaAlloc((arena), sizeof(type) * (count)))
