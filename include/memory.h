/*******************************************************************************************
*
*   memory.h — Memory Management System (Pool Allocators)
*
*   Manages all dynamic memory for the engine using pool-based allocation.
*   Instead of calling malloc/free for every actor and component (which causes
*   fragmentation and cache misses), we pre-allocate large blocks and sub-allocate.
*
*   Two allocator types (from rmem.h library):
*
*   1. ObjPool (for Actors):
*       Fixed-size object pool. All actors are the same size, so we use a pool
*       of pre-allocated Actor slots. O(1) alloc and free, zero fragmentation.
*       512 actors × sizeof(Actor) ≈ ~200 KB
*
*   2. MemPool (for Components):
*       Variable-size memory pool. Components vary in size (MeshComponent is bigger
*       than MoveComponent), so we use a general-purpose pool with a free list.
*       128 KB pool with bucket-based free list management.
*
*   Architecture:
*       Game
*           └── MemorySystem (embedded)
*                   ├── actorPool     (ObjPool — fixed-size slots)
*                   └── componentPool (MemPool — variable-size allocations)
*
*   Naming Convention:
*       API:     MEMORY_*
*
********************************************************************************************/
#pragma once

#include "rmem.h"
#include <stddef.h>

typedef struct MemorySystem MemorySystem;
typedef struct Actor Actor;

/* ── Pool Configuration ──────────────────────────────────────────────────── */
#define ACTOR_POOL_COUNT      512               /* Max actors in the pool         */
#define COMPONENT_POOL_BYTES  (128 * 1024)      /* 128 KB for component pool      */

struct MemorySystem
{
    ObjPool actorPool;          /* Fixed-size pool for Actor structs                  */
    MemPool componentPool;      /* Variable-size pool for all component types          */
    /* BiStack frameScratch; */ /* TODO: for instanced rendering, collision queries    */
};

/* ── Lifecycle ───────────────────────────────────────────────────────────── */
void MEMORY_Init(MemorySystem* memory);
void MEMORY_Shutdown(MemorySystem* memory);

/* ── Actor Pool ──────────────────────────────────────────────────────────── */
Actor* MEMORY_AllocActor(MemorySystem* memory);
void   MEMORY_FreeActor(MemorySystem* memory, Actor* actor);

/* ── Component Pool ──────────────────────────────────────────────────────── */
void* MEMORY_AllocComponent(MemorySystem* memory, size_t size);
void  MEMORY_FreeComponent(MemorySystem* memory, void* comp);

/* ── Diagnostics ─────────────────────────────────────────────────────────── */
int    MEMORY_GetActorPoolUsed(const MemorySystem* memory);
int    MEMORY_GetActorPoolTotal(const MemorySystem* memory);
size_t MEMORY_GetComponentPoolUsed(const MemorySystem* memory);
size_t MEMORY_GetComponentPoolTotal(const MemorySystem* memory);
size_t MEMORY_GetComponentPoolFree(const MemorySystem* memory);
int    MEMORY_GetComponentPoolFreeListLength(const MemorySystem* memory);