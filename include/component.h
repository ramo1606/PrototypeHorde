#pragma once

#include <stdbool.h>

typedef struct Actor Actor;
typedef struct Component Component;

typedef enum 
{
    COMPONENT_NONE = 0,
    COMPONENT_SCENE,
    COMPONENT_MESH,
    COMPONENT_MOVE,
    COMPONENT_CAMERA,
    COMPONENT_CAMERA_TPS,
    COMPONENT_BOX,
    COMPONENT_SPHERE,
    NUM_COMPONENT_TYPES
} ComponentType;

typedef void (*ComponentUpdateFn)(Component* self, float deltaTime);
typedef void (*ComponentInputFn)(Component* self);
typedef void (*ComponentDestroyFn)(Component* self);

struct Component 
{
    Actor        *owner;
    ComponentType type;
    int           updateOrder;

    ComponentUpdateFn    Update;
    ComponentInputFn     Input;
    ComponentDestroyFn   Destroy;
};

void COMPONENT_Init(Component* comp, Actor* owner, ComponentType type, int updateOrder);
void COMPONENT_Destroy(Component* comp);

const char* COMPONENT_GetTypeName(ComponentType type);

//void COMPONENT_LoadProperty(json);
//void COMPONENT_SaveProperty(json);
//Component* COMPONENT_Create(Actor* actor, json);