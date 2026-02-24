#pragma once

#include <stdbool.h>

typedef struct Actor Actor;
typedef struct Component Component;

typedef enum 
{
    COMPONENT_TYPE_NONE = 0,
    COMPONENT_TYPE_SCENE,
    COMPONENT_TYPE_MESH,
    COMPONENT_TYPE_MOVE,
    COMPONENT_TYPE_CAMERA,
    COMPONENT_TYPE_CAMERA_TPS,
    COMPONENT_TYPE_BOX,
    COMPONENT_TYPE_SPHERE,
    COMPONENT_TYPE_CAPSULE,         /* Phase 5 */
    COMPONENT_TYPE_MODEL,           /* Phase 7 */
    COMPONENT_TYPE_AUDIO_SOURCE,    /* Phase 8 */
    COMPONENT_TYPE_AI_CONTROLLER,   /* Phase 11 */
    COMPONENT_TYPE_FSM,             /* Phase 3 */
    NUM_COMPONENT_TYPES
} ComponentType;

typedef void (*ComponentUpdateFn)(Component* self, float deltaTime);
typedef void (*ComponentInputFn)(Component* self);
typedef void (*ComponentDestroyFn)(Component* self);

struct Component 
{
    Actor* owner;
    ComponentType type;
    int updateOrder;

    ComponentUpdateFn    Update;
    ComponentInputFn     Input;
    ComponentDestroyFn   Destroy;
};

void COMPONENT_Init(Component* comp, Actor* owner, ComponentType type, int updateOrder);
void COMPONENT_Destroy(Component* comp);

const char* COMPONENT_GetTypeName(ComponentType type);