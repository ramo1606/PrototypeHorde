/*******************************************************************************************
*
*   scene_component.h — Scene Component (Transform Hierarchy)
*
*   SceneComponent manages an entity's position, rotation, and scale in the world.
*   It supports a parent-child hierarchy: a child's world transform is computed by
*   multiplying its local transform with its parent's world transform.
*
*   This is the foundation for spatial relationships in the engine:
*       - An Actor's root is always a SceneComponent.
*       - Children automatically move/rotate with their parent.
*
*   Dirty Flag Pattern:
*       World transforms are cached and only recomputed when the dirty flag is set.
*       Setting position/rotation/scale marks the node AND all its descendants dirty.
*       ComputeWorldTransform checks the dirty flag and skips if already clean.
*
*   Fixed-Timestep Interpolation:
*       For smooth rendering between physics ticks, SceneComponent stores the previous
*       frame's position/rotation. The render loop interpolates between prev and current
*       using the accumulator alpha, then restores the actual values afterward.
*
*   Naming Convention:
*       API:     SCENE_COMPONENT_*
*
********************************************************************************************/

#pragma once

#include "component.h"
#include "raylib.h"
#include "raymath.h"

#define SCENE_MAX_CHILDREN 8

typedef struct Actor Actor;
typedef struct SceneComponent SceneComponent;

/* ── Scene Component Struct ──────────────────────────────────────────────── */

struct SceneComponent
{
    Component base;                 /* Inherited base component (must be first field) */

    Vector3 position;               /* Local position relative to parent              */
    Vector3 rotation;               /* Local rotation as Euler angles (radians, XYZ)  */
    Vector3 scale;                  /* Local scale per axis (default: 1,1,1)          */

    Vector3 prevPosition;           /* Position snapshot from start of frame           */
    Vector3 prevRotation;           /* Rotation snapshot from start of frame           */

    Matrix  localTransform;         /* S * R * T (scale, then rotate, then translate)  */
    Matrix  worldTransform;         /* localTransform * parent.worldTransform          */
    bool    isDirty;                /* True if localTransform needs recomputation      */

    SceneComponent* parent;                         /* Parent node (NULL for root)     */
    SceneComponent* children[SCENE_MAX_CHILDREN];   /* Child nodes                     */
    int             childCount;                     /* Current number of children      */
};

/* ── Initialization ──────────────────────────────────────────────────────── */
void SCENE_COMPONENT_InitRoot(SceneComponent* sc, Actor* owner);
void SCENE_COMPONENT_Init(SceneComponent* sc, Actor* owner, ComponentType type, int updateOrder);

/* ── Hierarchy Management ────────────────────────────────────────────────── */
void SCENE_COMPONENT_AttachChild(SceneComponent* parent, SceneComponent* child);
void SCENE_COMPONENT_DetachChild(SceneComponent* parent, SceneComponent* child);
void SCENE_COMPONENT_DetachFromParent(SceneComponent* sc);

/* ── Transform Computation ───────────────────────────────────────────────── */
void SCENE_COMPONENT_MarkDirty(SceneComponent* sc);
void SCENE_COMPONENT_ComputeWorldTransform(SceneComponent* sc);

/* ── Direction / Position Queries (World Space) ──────────────────────────── */
Vector3 SCENE_COMPONENT_GetForward(SceneComponent* sc);
Vector3 SCENE_COMPONENT_GetRight(SceneComponent* sc);
Vector3 SCENE_COMPONENT_GetUp(SceneComponent* sc);
Vector3 SCENE_COMPONENT_GetWorldPosition(SceneComponent* sc);
Matrix  SCENE_COMPONENT_GetWorldTransform(SceneComponent* sc);
float   SCENE_COMPONENT_GetWorldScale(SceneComponent* sc);

/* ── Owner Access ────────────────────────────────────────────────────────── */
Actor* SCENE_COMPONENT_GetOwner(SceneComponent* sc);

/* ── Fixed-Timestep Interpolation ────────────────────────────────────────── */
void SCENE_COMPONENT_SavePrevState(SceneComponent* sc);
void SCENE_COMPONENT_InterpolateForRender(SceneComponent* sc, float alpha);
void SCENE_COMPONENT_RestoreFromInterpolation(SceneComponent* sc);