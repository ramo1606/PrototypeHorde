#include "resource.h"
#include <assert.h>
#include <string.h>

typedef struct
{
    ResourceID   id;
    ResourceType type;
    const char*  path;
} ResourceEntry;

static const ResourceEntry resourceTable[] =
{
    { RES_MODEL_PLAYER,   RESTYPE_MODEL,   "assets/models/player.glb"      },
    { RES_MODEL_ZOMBIE,   RESTYPE_MODEL,   "assets/models/zombie.glb"      },
    { RES_MODEL_DEVIL,    RESTYPE_MODEL,   "assets/models/devil.glb"       },
    { RES_MODEL_MUMMY,    RESTYPE_MODEL,   "assets/models/mummy.glb"       },
    { RES_MODEL_TANK,     RESTYPE_MODEL,   "assets/models/tank.glb"        },
    { RES_MODEL_BARREL,   RESTYPE_MODEL,   "assets/models/barrel.glb"      },
    { RES_MODEL_CRATE,    RESTYPE_MODEL,   "assets/models/crate.glb"       },
    { RES_MODEL_TURRET,   RESTYPE_MODEL,   "assets/models/turret.glb"      },
    { RES_TEX_CROSSHAIR,  RESTYPE_TEXTURE, "assets/textures/crosshair.png" },
    { RES_SHADER_CEL,     RESTYPE_SHADER,  "assets/shaders/cel"            },
    { RES_SND_PISTOL,     RESTYPE_SOUND,   "assets/sounds/pistol.wav"      },
    { RES_SND_EXPLOSION,  RESTYPE_SOUND,   "assets/sounds/explosion.wav"   },
    { RES_FONT_UI,        RESTYPE_FONT,    "assets/fonts/ui_font.ttf"      },
};

#define TABLE_COUNT (int)(sizeof(resourceTable) / sizeof(resourceTable[0]))

static Model   models[RES_COUNT];
static Texture textures[RES_COUNT];
static Shader  shaders[RES_COUNT];
static Sound   sounds[RES_COUNT];
static Font    fonts[RES_COUNT];
static bool    loaded[RES_COUNT];

static const ResourceEntry* findEntry(ResourceID id)
{
    for (int i = 0; i < TABLE_COUNT; i++)
    {
        if (resourceTable[i].id == id)
        {
            return &resourceTable[i];
        }
    }
    return NULL;
}

void ResourceInit(void)
{
    memset(loaded, 0, sizeof(loaded));
}

void ResourceShutdown(void)
{
    for (int i = 0; i < RES_COUNT; i++)
    {
        if (loaded[i])
        {
            ResourceUnload((ResourceID)i);
        }
    }
}

void ResourceLoad(ResourceID id)
{
    if (loaded[id])
    {
        return;
    }

    const ResourceEntry* e = findEntry(id);
    assert(e && "ResourceID missing from resourceTable!");

    switch (e->type)
    {
        case RESTYPE_MODEL:
            models[id] = LoadModel(e->path);
            break;
        case RESTYPE_TEXTURE:
            textures[id] = LoadTexture(e->path);
            break;
        case RESTYPE_SHADER:
            shaders[id] = LoadShader(TextFormat("%s.vs", e->path), TextFormat("%s.fs", e->path));
            break;
        case RESTYPE_SOUND:
            sounds[id] = LoadSound(e->path);
            break;
        case RESTYPE_FONT:
            fonts[id] = LoadFont(e->path);
            break;
    }
    loaded[id] = true;
}

void ResourceUnload(ResourceID id)
{
    if (!loaded[id])
    {
        return;
    }

    const ResourceEntry* e = findEntry(id);
    if (!e)
    {
        return;
    }

    switch (e->type)
    {
        case RESTYPE_MODEL:
            UnloadModel(models[id]);
            break;
        case RESTYPE_TEXTURE:
            UnloadTexture(textures[id]);
            break;
        case RESTYPE_SHADER:
            UnloadShader(shaders[id]);
            break;
        case RESTYPE_SOUND:
            UnloadSound(sounds[id]);
            break;
        case RESTYPE_FONT:
            UnloadFont(fonts[id]);
            break;
    }
    loaded[id] = false;
}

void ResourceLoadGroup(const ResourceID* ids, int count)
{
    for (int i = 0; i < count; i++)
    {
        ResourceLoad(ids[i]);
    }
}

void ResourceUnloadGroup(const ResourceID* ids, int count)
{
    for (int i = 0; i < count; i++)
    {
        ResourceUnload(ids[i]);
    }
}

Model ResourceGetModel(ResourceID id)
{
    assert(loaded[id]);
    return models[id];
}

Texture ResourceGetTexture(ResourceID id)
{
    assert(loaded[id]);
    return textures[id];
}

Shader ResourceGetShader(ResourceID id)
{
    assert(loaded[id]);
    return shaders[id];
}

Sound ResourceGetSound(ResourceID id)
{
    assert(loaded[id]);
    return sounds[id];
}

Font ResourceGetFont(ResourceID id)
{
    assert(loaded[id]);
    return fonts[id];
}

bool ResourceIsLoaded(ResourceID id)
{
    return loaded[id];
}
