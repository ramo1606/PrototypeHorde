#pragma once

/*
 * component.h — Base Component type for the Actor-Component system.
 *
 * Every piece of behaviour attached to an Actor is a Component (or a struct
 * that embeds Component as its first field).  This file defines:
 *   - The ComponentType enum used for RTTI (run-time type identification).
 *   - The function-pointer callbacks that drive the per-frame update loop.
 *   - The Component struct itself, which is the "base class" for all
 *     concrete component types.
 *
 * Architecture position:
 *   Game → Actors[] → Component[] (sorted by updateOrder)
 *
 * C-style inheritance pattern:
 *   Embed `Component base;` as the first field of any concrete component
 *   struct so it can be safely cast to/from `Component*`.
 */

#include <stdbool.h>

typedef struct Actor Actor;
typedef struct Component Component;

/* ── Component Type Registry ───────────────────────────────────── */

typedef enum 
{
    COMPONENT_TYPE_NONE = 0,        /* Uninitialized / placeholder */
    COMPONENT_TYPE_SCENE,           /* SceneComponent — transform node */
    COMPONENT_TYPE_MESH,            /* MeshComponent — renderable geometry */
    COMPONENT_TYPE_MOVE,            /* MoveComponent — forward/angular/strafe movement */
    COMPONENT_TYPE_CAMERA,          /* CameraComponent — wraps Camera3D */
    COMPONENT_TYPE_CAMERA_TPS,      /* CameraTPS — spring-damper third-person camera */
    COMPONENT_TYPE_BOX,             /* BoxComponent — AABB collider */
    COMPONENT_TYPE_SPHERE,          /* SphereComponent — sphere collider */
    COMPONENT_TYPE_CAPSULE,         /* Phase 5 */
    COMPONENT_TYPE_MODEL,           /* Phase 7 */
    COMPONENT_TYPE_AUDIO_SOURCE,    /* Phase 8 */
    COMPONENT_TYPE_AI_CONTROLLER,   /* Phase 11 */
    COMPONENT_TYPE_FSM,             /* Phase 3 */
    NUM_COMPONENT_TYPES             /* Sentinel — number of registered types */
} ComponentType;

/* ── Per-frame Callbacks ────────────────────────────────────────── */

typedef void (*ComponentUpdateFn)(Component* self, float deltaTime);  /* Called once per fixed timestep when the actor is active */
typedef void (*ComponentInputFn)(Component* self);                     /* Called once per frame to process player/AI input */
typedef void (*ComponentDestroyFn)(Component* self);                   /* Called on component teardown before memory is freed */

/* ── Component Struct ───────────────────────────────────────────── */

struct Component 
{
    Actor*             owner;        /* Back-pointer to the actor that owns this component */
    ComponentType      type;         /* RTTI tag — used to identify and cast the component */
    int                updateOrder;  /* Lower values update first; used for sorted insertion */

    ComponentUpdateFn  Update;       /* Per-fixed-timestep update callback (NULL = no update) */
    ComponentInputFn   Input;        /* Per-frame input callback (NULL = no input handling) */
    ComponentDestroyFn Destroy;      /* Custom cleanup called before deallocation (NULL = no-op) */
};

/* ── Public API ─────────────────────────────────────────────────── */

void COMPONENT_Init(Component* comp, Actor* owner, ComponentType type, int updateOrder); // Initialise fields and register the component with its owner actor
void COMPONENT_Destroy(Component* comp);                                                  // Invoke Destroy callback, unregister from owner, and free component memory

const char* COMPONENT_GetTypeName(ComponentType type); // Return a human-readable name string for the given ComponentType