#pragma once

#include "raylib.h"
#include <stdbool.h>

typedef enum ResourceID 
{
    RES_MODEL_PLAYER,
    RES_MODEL_ZOMBIE,
    RES_MODEL_DEVIL,
    RES_MODEL_MUMMY,
    RES_MODEL_TANK,
    RES_MODEL_BARREL,
    RES_MODEL_CRATE,
    RES_MODEL_TURRET,
    RES_TEX_CROSSHAIR,
    RES_SHADER_CEL,
    RES_SND_PISTOL,
    RES_SND_EXPLOSION,
    RES_FONT_UI,
    RES_COUNT
} ResourceID;

typedef enum ResourceType 
{
    RESTYPE_MODEL,
    RESTYPE_TEXTURE,
    RESTYPE_SHADER,
    RESTYPE_SOUND,
    RESTYPE_FONT,
} ResourceType;

void    RESOURCE_Init(void);
void    RESOURCE_Shutdown(void);
void    RESOURCE_Load(ResourceID id);
void    RESOURCE_Unload(ResourceID id);
void    RESOURCE_LoadGroup(const ResourceID* ids, int count);
void    RESOURCE_UnloadGroup(const ResourceID* ids, int count);
Model   RESOURCE_GetModel(ResourceID id);
Texture RESOURCE_GetTexture(ResourceID id);
Shader  RESOURCE_GetShader(ResourceID id);
Sound   RESOURCE_GetSound(ResourceID id);
Font    RESOURCE_GetFont(ResourceID id);
bool    RESOURCE_IsLoaded(ResourceID id);
