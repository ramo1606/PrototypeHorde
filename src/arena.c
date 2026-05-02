#include "arena.h"

MemArena ArenaCreate(size_t capacity)            { return CreateMemPool(capacity); }
void     ArenaDestroy(MemArena* arena)           { DestroyMemPool(arena); }
void     ArenaReset(MemArena* arena)             { MemPoolReset(arena); }

void*    ArenaAlloc(MemArena* arena, size_t size){ return MemPoolAlloc(arena, size); }
void     ArenaFree(MemArena* arena, void* ptr)   { MemPoolFree(arena, ptr); }

size_t   ArenaGetFreeMemory(MemArena arena)      { return GetMemPoolFreeMemory(arena); }
