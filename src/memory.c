/*******************************************************************************************
*
*   memory.c — Memory Management System Implementation
*
********************************************************************************************/

#include "memory.h"

#define RMEM_IMPLEMENTATION
#include "rmem.h"
#include "actor.h"
#include "raylib.h"
#include <assert.h>

/*------------------------------------------------------------------------------------
 * MEMORY_Init
 *
 *   Creates both memory pools:
 *   1. Actor ObjPool: pre-allocates ACTOR_POOL_COUNT slots of sizeof(Actor) each.
 *      ObjPool uses a free-list internally for O(1) alloc/free with zero fragmentation.
 *   2. Component MemPool: pre-allocates COMPONENT_POOL_BYTES of general-purpose
 *      memory with bucket-based free list management for variable-size allocations.
 *
 *   If either pool fails to allocate, the other is cleaned up and an error is logged.
 *   The rmem library handles the underlying malloc and bookkeeping.
 *------------------------------------------------------------------------------------*/

void MEMORY_Init(MemorySystem* memory)
{
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

/*------------------------------------------------------------------------------------
 * MEMORY_Shutdown
 *
 *   Destroys both pools, freeing the backing memory.
 *   Component pool is freed first (components reference actors, not vice versa).
 *------------------------------------------------------------------------------------*/

void MEMORY_Shutdown(MemorySystem* memory)
{
    assert(memory != NULL);

    DestroyMemPool(&memory->componentPool);
    DestroyObjPool(&memory->actorPool);

    TraceLog(LOG_INFO, "MEMORY: All pools destroyed");
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Actor Pool — Fixed-Size Object Pool
 *
 *  ObjPool works like a stack of pre-allocated slots:
 *  - Alloc: pop the next free slot off the free list → O(1)
 *  - Free: push the slot back onto the free list → O(1)
 *  - No fragmentation since all slots are the same size
 *  - Cache-friendly since actors are contiguous in memory
 * ═══════════════════════════════════════════════════════════════════════════ */

 /* Allocate one Actor slot. Returns NULL if all slots are in use. */
Actor* MEMORY_AllocActor(MemorySystem* memory)
{
    assert(memory != NULL);

    Actor* actor = (Actor*)ObjPoolAlloc(&memory->actorPool);
    if (!actor)
    {
        TraceLog(LOG_WARNING, "MEMORY: Actor pool exhausted (%d slots)",
            ACTOR_POOL_COUNT);
    }
    return actor;
}

/* Return an Actor slot to the pool for reuse. */
void MEMORY_FreeActor(MemorySystem* memory, Actor* actor)
{
    assert(memory != NULL);
    if (actor) ObjPoolFree(&memory->actorPool, actor);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Component Pool — Variable-Size Memory Pool
 *
 *  MemPool handles allocations of varying sizes using a free-list approach:
 *  - Small allocations go into size-bucketed free lists for fast reuse
 *  - Large allocations use a general free list
 *  - All memory comes from a single pre-allocated arena
 *  - More fragmentation possible than ObjPool, but necessary for varying sizes
 * ═══════════════════════════════════════════════════════════════════════════ */

 /* Allocate 'size' bytes for a component. Returns NULL if pool exhausted. */
void* MEMORY_AllocComponent(MemorySystem* memory, size_t size)
{
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

/* Return component memory to the pool. */
void MEMORY_FreeComponent(MemorySystem* memory, void* comp)
{
    assert(memory != NULL);
    if (comp) MemPoolFree(&memory->componentPool, comp);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Diagnostics
 *
 *  These functions provide runtime insight into pool utilization.
 *  Useful for the debug overlay (DEBUG_Render) and for tuning pool sizes.
 * ═══════════════════════════════════════════════════════════════════════════ */

 /* Actor slots in use = total slots - free slots. */
int MEMORY_GetActorPoolUsed(const MemorySystem* memory)
{
    assert(memory != NULL);
    return (int)(memory->actorPool.memSize - memory->actorPool.freeBlocks);
}

/* Total actor slots available in the pool. */
int MEMORY_GetActorPoolTotal(const MemorySystem* memory)
{
    assert(memory != NULL);
    return (int)memory->actorPool.memSize;
}

/* Bytes used = total arena size - free memory reported by pool. */
size_t MEMORY_GetComponentPoolUsed(const MemorySystem* memory)
{
    assert(memory != NULL);
    size_t total = memory->componentPool.arena.size;
    size_t free = GetMemPoolFreeMemory(memory->componentPool);
    return total - free;
}

/* Total bytes in the component pool arena. */
size_t MEMORY_GetComponentPoolTotal(const MemorySystem* memory)
{
    assert(memory != NULL);
    return memory->componentPool.arena.size;
}

/* Free bytes remaining in the component pool. */
size_t MEMORY_GetComponentPoolFree(const MemorySystem* memory)
{
    assert(memory != NULL);
    return GetMemPoolFreeMemory(memory->componentPool);
}

/*------------------------------------------------------------------------------------
 * MEMORY_GetComponentPoolFreeListLength
 *
 *   Returns the total number of entries across all free-list buckets.
 *   A high count with low memory usage suggests fragmentation.
 *   MemPool uses MEMPOOL_BUCKET_SIZE buckets for different allocation sizes,
 *   plus a 'large' list for oversized allocations.
 *------------------------------------------------------------------------------------*/
int MEMORY_GetComponentPoolFreeListLength(const MemorySystem* memory)
{
    assert(memory != NULL);
    int count = (int)memory->componentPool.large.len;
    for (int i = 0; i < MEMPOOL_BUCKET_SIZE; i++)
    {
        count += (int)memory->componentPool.buckets[i].len;
    }
    return count;
}