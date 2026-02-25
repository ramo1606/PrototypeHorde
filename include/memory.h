#pragma once

/*
 * memory.h — Pool allocator system for actors and components.
 *
 * MemorySystem wraps two allocators from the rmem library:
 *
 *   - ObjPool (actorPool): Fixed-size slab allocator for Actor structs.
 *     All Actor slots are the same size, so alloc/free are O(1) with no
 *     fragmentation.  Capacity is ACTOR_POOL_COUNT slots.
 *
 *   - MemPool (componentPool): Variable-size general allocator backed by a
 *     contiguous memory arena with a free-list.  Used for all component
 *     types (which differ in size).  Capacity is COMPONENT_POOL_BYTES.
 *
 * Using pool allocators avoids heap fragmentation and keeps allocations
 * cache-friendly — all Actor memory is in one contiguous slab.
 *
 * Architecture position:
 *   Game.memory (value, not pointer)
 *   Used by ACTOR_Create/Destroy and all COMPONENT_Create functions.
 */

#include "rmem.h"
#include <stddef.h>

typedef struct Actor Actor;

/* ── Pool Capacity Constants ────────────────────────────────────── */

#define ACTOR_POOL_COUNT      512                    /* Maximum number of Actor slots in the object pool */
#define COMPONENT_POOL_BYTES  (128 * 1024)           /* 128 KB *//* Total byte capacity of the component memory pool */

/* ── MemorySystem Struct ─────────────────────────────────────────── */

typedef struct 
{
    ObjPool actorPool;       /* Fixed-size slab pool for Actor objects (ACTOR_POOL_COUNT slots of sizeof(Actor)) */
    MemPool componentPool;   /* Variable-size memory pool for all component types (COMPONENT_POOL_BYTES total) */
    /* BiStack frameScratch; */ /* TODO: for instanced rendering, collision queries */
} MemorySystem;

/* ── Lifecycle ──────────────────────────────────────────────────── */

void MEMORY_Init(MemorySystem* memory);      // Allocate and initialise both the actor pool and the component pool
void MEMORY_Shutdown(MemorySystem* memory);  // Destroy both pools and release their backing memory

/* ── Actor Allocation ───────────────────────────────────────────── */

Actor* MEMORY_AllocActor(MemorySystem* memory);              // Allocate one Actor slot from the ObjPool; returns NULL if pool is exhausted
void   MEMORY_FreeActor(MemorySystem* memory, Actor* actor); // Return an Actor slot to the ObjPool

/* ── Component Allocation ───────────────────────────────────────── */

void* MEMORY_AllocComponent(MemorySystem* memory, size_t size); // Allocate size bytes from the MemPool; returns NULL if pool is exhausted
void  MEMORY_FreeComponent(MemorySystem* memory, void* comp);   // Return a component allocation to the MemPool free-list

/* ── Diagnostics ────────────────────────────────────────────────── */

int    MEMORY_GetActorPoolUsed(const MemorySystem* memory);              // Return the number of Actor slots currently in use
int    MEMORY_GetActorPoolTotal(const MemorySystem* memory);             // Return the total number of Actor slots in the pool
size_t MEMORY_GetComponentPoolUsed(const MemorySystem* memory);          // Return the number of bytes currently allocated from the component pool
size_t MEMORY_GetComponentPoolTotal(const MemorySystem* memory);         // Return the total byte capacity of the component pool
size_t MEMORY_GetComponentPoolFree(const MemorySystem* memory);          // Return the number of bytes currently free in the component pool
int    MEMORY_GetComponentPoolFreeListLength(const MemorySystem* memory); // Return the total number of free-list entries (bucket + large) in the component pool