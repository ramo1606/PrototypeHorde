#include "memory.h"

#define RMEM_IMPLEMENTATION
#include "rmem.h"
#include "actor.h"
#include "raylib.h"
#include <assert.h>

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

void MEMORY_Shutdown(MemorySystem* memory)
{
    assert(memory != NULL);

    DestroyMemPool(&memory->componentPool);
    DestroyObjPool(&memory->actorPool);

    TraceLog(LOG_INFO, "MEMORY: All pools destroyed");
}

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

void MEMORY_FreeActor(MemorySystem* memory, Actor* actor)
{
    assert(memory != NULL);
    if (actor) ObjPoolFree(&memory->actorPool, actor);
}

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

void MEMORY_FreeComponent(MemorySystem* memory, void* comp)
{
    assert(memory != NULL);
    if (comp) MemPoolFree(&memory->componentPool, comp);
}