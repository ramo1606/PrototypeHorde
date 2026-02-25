#include "memory.h"

#define RMEM_IMPLEMENTATION
#include "rmem.h"
#include "actor.h"
#include "raylib.h"
#include <assert.h>

void MEMORY_Init(MemorySystem* memory)
{
    /*
     * Create both pool allocators.  The rmem library allocates each pool
     * from the system heap in a single contiguous block.
     *
     * actorPool  — ObjPool: fixed-size slab allocator for Actor structs.
     *              All slots are sizeof(Actor) bytes; alloc/free are O(1).
     *
     * componentPool — MemPool: general-purpose variable-size allocator
     *              backed by a COMPONENT_POOL_BYTES arena with a free-list
     *              of buckets.  Used for all component types.
     *
     * If either pool fails to allocate (out-of-memory), the error is
     * logged and the partially-initialised allocator is cleaned up.
     */
    assert(memory != NULL);

    memory->actorPool = CreateObjPool(sizeof(Actor), ACTOR_POOL_COUNT);
    if (memory->actorPool.mem == 0)
    {
        TraceLog(LOG_ERROR, "MEMORY: Failed to create actor pool");
        return;
    }

    memory->componentPool = CreateMemPool(COMPONENT_POOL_BYTES);
    if (memory->componentPool.arena.mem == 0)
    {
        TraceLog(LOG_ERROR, "MEMORY: Failed to create component pool");
        DestroyObjPool(&memory->actorPool);
        return;
    }

    TraceLog(LOG_INFO, "MEMORY: Actor pool: %d slots (%zu bytes each)",
        ACTOR_POOL_COUNT, sizeof(Actor));
    TraceLog(LOG_INFO, "MEMORY: Component pool: %d KB",
        COMPONENT_POOL_BYTES / 1024);
}

void MEMORY_Shutdown(MemorySystem* memory)
{
    /*
     * Destroy both pools in reverse creation order.  This releases the
     * underlying heap allocations made by the rmem library.
     * All actors and components must already be freed before this is called
     * (GAME_Shutdown ensures that via GAME_RemoveAllActors).
     */
    assert(memory != NULL);

    DestroyMemPool(&memory->componentPool);
    DestroyObjPool(&memory->actorPool);

    TraceLog(LOG_INFO, "MEMORY: All pools destroyed");
}

Actor* MEMORY_AllocActor(MemorySystem* memory)
{
    /*
     * Claim one fixed-size slot from the ObjPool.  ObjPool internally
     * manages a free-list of pre-allocated slots, so this is O(1) with
     * no fragmentation.  Returns NULL if ACTOR_POOL_COUNT is exhausted.
     */
    assert(memory != NULL);

    Actor* actor = (Actor*)ObjPoolAlloc(&memory->actorPool);
    if (!actor)
    {
        TraceLog(LOG_WARNING, "MEMORY: Actor pool exhausted (%d slots)",
            ACTOR_POOL_COUNT);
    }
    return actor;
}

void MEMORY_FreeActor(MemorySystem* memory, Actor* actor)
{
    /*
     * Return the slot to the ObjPool free-list.  O(1) operation.
     * Silently ignores NULL actors.
     */
    assert(memory != NULL);
    if (actor) ObjPoolFree(&memory->actorPool, actor);
}

void* MEMORY_AllocComponent(MemorySystem* memory, size_t size)
{
    /*
     * Allocate size bytes from the MemPool.  The MemPool uses a
     * bucket free-list for small allocations and a large-block list for
     * allocations that exceed the bucket threshold.  Returns NULL if the
     * pool is exhausted.
     */
    assert(memory != NULL);
    assert(size > 0);

    void* comp = MemPoolAlloc(&memory->componentPool, size);
    if (!comp)
    {
        TraceLog(LOG_WARNING, "MEMORY: Component pool exhausted (%zu bytes requested)",
            size);
    }
    return comp;
}

void MEMORY_FreeComponent(MemorySystem* memory, void* comp)
{
    /*
     * Return the allocation to the MemPool free-list.  The pool places
     * it back in the appropriate bucket or large-block list based on size.
     * Silently ignores NULL pointers.
     */
    assert(memory != NULL);
    if (comp) MemPoolFree(&memory->componentPool, comp);
}

int MEMORY_GetActorPoolUsed(const MemorySystem* memory)
{
    assert(memory != NULL);
    return (int)(memory->actorPool.memSize - memory->actorPool.freeBlocks);
}

int MEMORY_GetActorPoolTotal(const MemorySystem* memory)
{
    assert(memory != NULL);
    return (int)memory->actorPool.memSize;
}

size_t MEMORY_GetComponentPoolUsed(const MemorySystem* memory)
{
    assert(memory != NULL);
    size_t total = memory->componentPool.arena.size;
    size_t free = GetMemPoolFreeMemory(memory->componentPool);
    return total - free;
}

size_t MEMORY_GetComponentPoolTotal(const MemorySystem* memory)
{
    assert(memory != NULL);
    return memory->componentPool.arena.size;
}

size_t MEMORY_GetComponentPoolFree(const MemorySystem* memory)
{
    assert(memory != NULL);
    return GetMemPoolFreeMemory(memory->componentPool);
}

int MEMORY_GetComponentPoolFreeListLength(const MemorySystem* memory)
{
    /*
     * Count the total number of free-list entries across all bucket sizes
     * plus the large-block list.  Useful for diagnosing fragmentation.
     */
    assert(memory != NULL);
    int count = (int)memory->componentPool.large.len;
    for (int i = 0; i < MEMPOOL_BUCKET_SIZE; i++)
    {
        count += (int)memory->componentPool.buckets[i].len;
    }
    return count;
}