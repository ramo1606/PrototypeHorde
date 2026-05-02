#pragma once

#include "resource_types.h"
#include "raylib.h"
#include <stdbool.h>

void    ResourceInit(void);
void    ResourceShutdown(void);

void    ResourceLoad(ResourceID id);
void    ResourceUnload(ResourceID id);
void    ResourceLoadGroup(const ResourceID* ids, int count);
void    ResourceUnloadGroup(const ResourceID* ids, int count);

Model   ResourceGetModel(ResourceID id);
Texture ResourceGetTexture(ResourceID id);
Shader  ResourceGetShader(ResourceID id);
Sound   ResourceGetSound(ResourceID id);
Font    ResourceGetFont(ResourceID id);

bool    ResourceIsLoaded(ResourceID id);
