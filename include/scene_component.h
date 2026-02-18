#pragma once

#include "component.h"
#include "raylib.h"
#include "raymath.h"

#define SCENE_MAX_CHILDREN 8

typedef struct Actor Actor;
typedef struct SceneComponent SceneComponent;

struct SceneComponent
{
    Component base;

    /* Local transform */
    Vector3 position;
    Vector3 rotation;       /* Euler angles in radians (pitch=X, yaw=Y, roll=Z) */
    Vector3 scale;

    /* Computed */
    Matrix  localTransform;
    Matrix  worldTransform;
    bool    isDirty;

    /* Hierarchy */
    SceneComponent* parent;
    SceneComponent* children[SCENE_MAX_CHILDREN];
    int             childCount;
};

/* Init root — for Actor's embedded root, does NOT add to component list */
void SCENE_COMPONENT_InitRoot(SceneComponent* sc, Actor* owner);

/* Init as component — for MeshComponent etc, DOES add to actor's component list */
void SCENE_COMPONENT_Init(SceneComponent* sc, Actor* owner, ComponentType type, int updateOrder);

/* Hierarchy */
void SCENE_COMPONENT_AttachChild(SceneComponent* parent, SceneComponent* child);
void SCENE_COMPONENT_DetachChild(SceneComponent* parent, SceneComponent* child);

/* Dirty flag — propagates to all descendants */
void SCENE_COMPONENT_MarkDirty(SceneComponent* sc);

/* Lazy world transform resolve (no-op if clean, resolves parent first) */
void SCENE_COMPONENT_ComputeWorldTransform(SceneComponent* sc);

/* Direction helpers (auto-resolve if dirty) */
Vector3 SCENE_COMPONENT_GetForward(SceneComponent* sc);
Vector3 SCENE_COMPONENT_GetRight(SceneComponent* sc);
Vector3 SCENE_COMPONENT_GetUp(SceneComponent* sc);
Vector3 SCENE_COMPONENT_GetWorldPosition(SceneComponent* sc);

Actor* SCENE_COMPONENT_GetOwner(SceneComponent* sc);