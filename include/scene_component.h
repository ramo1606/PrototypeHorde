#pragma once

/*
 * scene_component.h — Transform-hierarchy node (SceneComponent).
 *
 * SceneComponent extends Component by adding 3-D transform data (position,
 * rotation, scale) and a parent/child tree so that objects can be arranged
 * in a scene graph.  It is the "root" component that every Actor owns, and
 * it is also embedded as the first field of MeshComponent and
 * CameraComponent so those can participate in the same hierarchy.
 *
 * Key design:
 *   - Dirty-flag pattern: worldTransform is only recomputed when isDirty is
 *     set, propagating lazily down the child list.
 *   - Visual interpolation: prevPosition/prevRotation snapshot is taken
 *     before the fixed update; the render step lerps between them for
 *     sub-frame smoothness.
 *
 * Architecture position:
 *   Actor.root (SceneComponent) ← parent of all scene-attached children
 *       └── MeshComponent.scene
 *       └── CameraComponent.scene
 */

#include "component.h"
#include "raylib.h"
#include "raymath.h"

#define SCENE_MAX_CHILDREN 8  /* Maximum direct children a SceneComponent may have */

typedef struct Actor Actor;
typedef struct SceneComponent SceneComponent;

/* ── SceneComponent Struct ──────────────────────────────────────── */

struct SceneComponent
{
    Component base;               /* Embedded Component — must be first field for safe casting */

    Vector3 position;             /* Local-space position relative to parent (or world if no parent) */
    Vector3 rotation;             /* Local-space Euler angles in radians (XYZ order) */
    Vector3 scale;                /* Local-space non-uniform scale */

    Vector3 prevPosition;         /* Position snapshot saved before the fixed update (used for visual interpolation) */
    Vector3 prevRotation;         /* Rotation snapshot saved before the fixed update (used for visual interpolation) */

    Matrix  localTransform;       /* Cached SRT matrix in local space (recomputed when dirty) */
    Matrix  worldTransform;       /* Cached world-space transform (parent * local, recomputed when dirty) */
    bool    isDirty;              /* True when localTransform / worldTransform need to be recomputed */

    SceneComponent* parent;                       /* Parent node in the scene graph (NULL = world root) */
    SceneComponent* children[SCENE_MAX_CHILDREN]; /* Array of direct child nodes */
    int             childCount;                   /* Number of valid entries in children[] */
};

/* ── Initialisation ─────────────────────────────────────────────── */

void SCENE_COMPONENT_InitRoot(SceneComponent* sc, Actor* owner);                                     // Initialise as an actor's root node (not registered in component list)
void SCENE_COMPONENT_Init(SceneComponent* sc, Actor* owner, ComponentType type, int updateOrder);    // Initialise as a regular component and register it with the owner actor

/* ── Hierarchy Management ───────────────────────────────────────── */

void SCENE_COMPONENT_AttachChild(SceneComponent* parent, SceneComponent* child);   // Attach child to parent; detaches from any previous parent first
void SCENE_COMPONENT_DetachChild(SceneComponent* parent, SceneComponent* child);   // Detach a specific child from this parent node
void SCENE_COMPONENT_DetachFromParent(SceneComponent* sc);                         // Detach this node from its current parent (if any)

/* ── Transform ──────────────────────────────────────────────────── */

void SCENE_COMPONENT_MarkDirty(SceneComponent* sc);               // Mark this node and all descendants as needing a transform recompute
void SCENE_COMPONENT_ComputeWorldTransform(SceneComponent* sc);   // Recompute localTransform and worldTransform if dirty; recurses up the parent chain first

Vector3 SCENE_COMPONENT_GetForward(SceneComponent* sc);           // Return the world-space forward vector (−Z column of worldTransform)
Vector3 SCENE_COMPONENT_GetRight(SceneComponent* sc);             // Return the world-space right vector (+X column of worldTransform)
Vector3 SCENE_COMPONENT_GetUp(SceneComponent* sc);                // Return the world-space up vector (+Y column of worldTransform)
Vector3 SCENE_COMPONENT_GetWorldPosition(SceneComponent* sc);     // Return the world-space translation (column 3 of worldTransform)
Matrix  SCENE_COMPONENT_GetWorldTransform(SceneComponent* sc);    // Return the full world-space transform matrix (recomputes if dirty)
float   SCENE_COMPONENT_GetWorldScale(SceneComponent* sc);        // Return the uniform world-space scale (magnitude of first column)

/* ── Ownership ──────────────────────────────────────────────────── */

Actor* SCENE_COMPONENT_GetOwner(SceneComponent* sc);  // Return the actor that owns this component via the embedded base

/* ── Visual Interpolation ───────────────────────────────────────── */

void SCENE_COMPONENT_SavePrevState(SceneComponent* sc);                     // Snapshot position/rotation into prevPosition/prevRotation before the fixed update
void SCENE_COMPONENT_InterpolateForRender(SceneComponent* sc, float alpha); // Lerp position/rotation between prev and current by alpha for smooth rendering
void SCENE_COMPONENT_RestoreFromInterpolation(SceneComponent* sc);          // Restore position/rotation from the snapshot taken during interpolation