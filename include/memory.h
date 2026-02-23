#pragma once

#include "rmem.h"
#include <stddef.h>

typedef struct Actor Actor;

/* Pool sizing */
#define ACTOR_POOL_COUNT      512
#define COMPONENT_POOL_BYTES  (128 * 1024)  /* 128 KB */

typedef struct 
{
    ObjPool actorPool;
    MemPool componentPool;
    /* BiStack frameScratch; */ /* TODO: for instanced rendering, collision queries */
} MemorySystem;

void MEMORY_Init(MemorySystem* memory);
void MEMORY_Shutdown(MemorySystem* memory);

/* Actor allocation — fixed size, O(1) alloc/free */
Actor* MEMORY_AllocActor(MemorySystem* memory);
void   MEMORY_FreeActor(MemorySystem* memory, Actor* actor);

/* Component allocation — variable size */
void* MEMORY_AllocComponent(MemorySystem* memory, size_t size);
void  MEMORY_FreeComponent(MemorySystem* memory, void* comp);

/* Stats */
int    MEMORY_GetActorPoolUsed(const MemorySystem* memory);
int    MEMORY_GetActorPoolTotal(const MemorySystem* memory);
size_t MEMORY_GetComponentPoolUsed(const MemorySystem* memory);
size_t MEMORY_GetComponentPoolTotal(const MemorySystem* memory);
size_t MEMORY_GetComponentPoolFree(const MemorySystem* memory);
int    MEMORY_GetComponentPoolFreeListLength(const MemorySystem* memory);