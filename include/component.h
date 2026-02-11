#pragma once

#include <stdbool.h>

typedef struct Actor Actor;
typedef struct Component Component;

typedef enum 
{
    COMPONENT_NONE = 0,
    COMPONENT_SPIN,
    COMPONENT_MESH,
    COMPONENT_MOVE,
    COMPONENT_FOLLOW_CAMERA,
    COMPONENT_BOX,
    COMPONENT_SKELETAL_MESH,
    COMPONENT_AUDIO,
    COMPONENT_TYPE_COUNT,
} ComponentType;

typedef void (*ComponentUpdateFn)(Component *self, float deltaTime);
typedef void (*ComponentInputFn)(Component *self);
typedef void (*ComponentTransformFn)(Component *self);
typedef void (*ComponentDestroyFn)(Component *self);

struct Component 
{
    Actor        *owner;
    ComponentType type;
    int           updateOrder;

    ComponentUpdateFn    onUpdate;
    ComponentInputFn     onInput;
    ComponentTransformFn onWorldTransform;
    ComponentDestroyFn   onDestroy;
};

void COMPONENT_Init(Component *comp, Actor *owner, ComponentType type, int updateOrder);
void COMPONENT_Destroy(Component *comp);