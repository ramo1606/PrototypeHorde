/*******************************************************************************************
*
*   component.h — Base Component
*
*   This is the root of the Actor-Component System. Every component in the engine
*   inherits from Component by embedding it as the first field of its struct (C-style
*   inheritance via composition). This allows safe casting between Component* and any
*   derived component pointer.
*
*   Lifecycle:
*       COMPONENT_Init()     → Registers component with its owning Actor
*       comp->Update()       → Called every fixed timestep (if set)
*       comp->Input()        → Called every frame for input processing (if set)
*       COMPONENT_Destroy()  → Calls custom Destroy callback, then frees memory
*
*   Naming Convention:
*       Enums:   COMPONENT_TYPE_*
*       API:     COMPONENT_*
*
********************************************************************************************/
#pragma once

#include <stdbool.h>

typedef struct Actor Actor;
typedef struct Component Component;

/* ── Component Type Registry ─────────────────────────────────────────────── */
typedef enum
{
    COMPONENT_TYPE_NONE = 0,            /* Uninitialized / invalid                        */
    COMPONENT_TYPE_SCENE,               /* SceneComponent — transform node                */
    COMPONENT_TYPE_MESH,                /* MeshComponent — renderable mesh                */
    COMPONENT_TYPE_MOVE,                /* MoveComponent — basic angular/linear movement  */
    COMPONENT_TYPE_CAMERA,              /* CameraComponent — base camera                  */
    COMPONENT_TYPE_CAMERA_TPS,          /* CameraTPS — third-person spring camera         */
    COMPONENT_TYPE_BOX,                 /* BoxComponent — AABB collider                   */
    COMPONENT_TYPE_SPHERE,              /* SphereComponent — sphere collider              */
    COMPONENT_TYPE_CAPSULE,             /* Phase 5: CapsuleComponent — capsule collider   */
    COMPONENT_TYPE_MODEL,               /* Phase 7: ModelComponent — animated 3D model    */
    COMPONENT_TYPE_AUDIO_SOURCE,        /* Phase 8: AudioSourceComponent — 3D audio       */
    COMPONENT_TYPE_AI_CONTROLLER,       /* Phase 11: AIController — enemy AI              */
    COMPONENT_TYPE_FSM,                 /* Phase 3: FSMComponent — finite state machine   */
    NUM_COMPONENT_TYPES                 /* Sentinel — total count of component types      */
} ComponentType;

/* ── Virtual Function Pointers ───────────────────────────────────────────── */
typedef void (*ComponentUpdateFn)(Component* self, float deltaTime);
typedef void (*ComponentInputFn)(Component* self);
typedef void (*ComponentDestroyFn)(Component* self);

/* ── Component Base Struct ───────────────────────────────────────────────── */
struct Component
{
    Actor* owner;                   /* Actor that owns this component (never NULL after Init) */
    ComponentType type;             /* Runtime type identifier for safe casting               */
    int updateOrder;                /* Sort priority: lower values update first               */

    ComponentUpdateFn Update;       /* Per-tick logic callback (NULL = no update)             */
    ComponentInputFn Input;         /* Per-frame input callback (NULL = no input)             */
    ComponentDestroyFn Destroy;     /* Cleanup callback invoked before memory is freed        */
};

/* ── Public API ──────────────────────────────────────────────────────────── */
void COMPONENT_Init(Component* comp, Actor* owner, ComponentType type, int updateOrder);
void COMPONENT_Destroy(Component* comp);

const char* COMPONENT_GetTypeName(ComponentType type);