/*******************************************************************************************
*
*   scene_component.c — Scene Component Implementation
*
********************************************************************************************/
#include "scene_component.h"
#include "actor.h"
#include <stddef.h>
#include <assert.h>

/*------------------------------------------------------------------------------------
 * SCENE_COMPONENT_InitFields (static)
 *
 *   Resets all transform fields to identity defaults:
 *   - Position = origin, Rotation = none, Scale = (1,1,1)
 *   - Transforms = identity matrix, isDirty = true (will recompute on first use)
 *   - No parent, no children
 *------------------------------------------------------------------------------------*/
static void SCENE_COMPONENT_InitFields(SceneComponent* sc)
{
    sc->position = (Vector3){ 0 };
    sc->rotation = (Vector3){ 0 };
    sc->scale    = (Vector3){ 1.0f, 1.0f, 1.0f };

    sc->prevPosition = (Vector3){ 0 };
    sc->prevRotation = (Vector3){ 0 };

    sc->localTransform = MatrixIdentity();
    sc->worldTransform = MatrixIdentity();
    sc->isDirty = true;

    sc->parent     = NULL;
    sc->childCount = 0;
    for (int i = 0; i < SCENE_MAX_CHILDREN; i++)
    {
        sc->children[i] = NULL;
    }
}

/*------------------------------------------------------------------------------------
 * SCENE_COMPONENT_InitRoot
 *
 *   Special initialization for the Actor embedded root SceneComponent.
 *   Unlike regular components, the root is NOT allocated from the component pool
 *   and NOT registered via ACTOR_AddComponent. We manually set the base fields
 *   to avoid the normal COMPONENT_Init path.
 *------------------------------------------------------------------------------------*/
void SCENE_COMPONENT_InitRoot(SceneComponent* sc, Actor* owner)
{
    assert(sc != NULL);
    assert(owner != NULL);

    /* Manual base init — skip COMPONENT_Init to avoid pool registration */
    sc->base.owner       = owner;
    sc->base.type        = COMPONENT_TYPE_SCENE;
    sc->base.updateOrder = 0;
    sc->base.Update      = NULL;
    sc->base.Input       = NULL;
    sc->base.Destroy     = NULL;

    SCENE_COMPONENT_InitFields(sc);
}

/*------------------------------------------------------------------------------------
 * SCENE_COMPONENT_Init
 *
 *   Standard initialization for heap-allocated SceneComponents.
 *   Calls COMPONENT_Init to register with the Actor's component list, then resets
 *   all transform fields.
 *------------------------------------------------------------------------------------*/

void SCENE_COMPONENT_Init(SceneComponent* sc, Actor* owner, ComponentType type, int updateOrder)
{
    assert(sc != NULL);
    assert(owner != NULL);

    COMPONENT_Init(&sc->base, owner, type, updateOrder);
    SCENE_COMPONENT_InitFields(sc);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Hierarchy Management
 *
 *  The scene hierarchy forms a tree rooted at each Actor root SceneComponent.
 *  MeshComponents and CameraComponents attach as children of the root.
 * ═══════════════════════════════════════════════════════════════════════════ */

 /*------------------------------------------------------------------------------------
  * SCENE_COMPONENT_AttachChild
  *
  *   Attaches a child to a parent in the hierarchy.
  *   If the child already has a parent, it is first detached (reparenting).
  *   The child is marked dirty since its world transform depends on the new parent.
  *------------------------------------------------------------------------------------*/
void SCENE_COMPONENT_AttachChild(SceneComponent* parent, SceneComponent* child)
{
    assert(parent != NULL);
    assert(child != NULL);

    /* Detach from previous parent if reparenting */
    if (child->parent)
    {
        SCENE_COMPONENT_DetachChild(child->parent, child);
    }

    if (parent->childCount >= SCENE_MAX_CHILDREN)
    {
        TraceLog(LOG_WARNING, "SCENE: Children full (%d)", SCENE_MAX_CHILDREN);
        return;
    }

    parent->children[parent->childCount++] = child;
    child->parent = parent;
    SCENE_COMPONENT_MarkDirty(child);
}

/*------------------------------------------------------------------------------------
 * SCENE_COMPONENT_DetachChild
 *
 *   Removes a child from a parent children array.
 *   Uses swap-with-last removal for O(1) — child order doesnt matter for transforms.
 *------------------------------------------------------------------------------------*/
void SCENE_COMPONENT_DetachChild(SceneComponent* parent, SceneComponent* child)
{
    assert(parent != NULL);
    assert(child != NULL);

    for (int i = 0; i < parent->childCount; i++)
    {
        if (parent->children[i] == child)
        {
            /* Swap with last element and shrink (O(1) removal) */
            parent->children[i] = parent->children[parent->childCount - 1];
            parent->children[parent->childCount - 1] = NULL;
            parent->childCount--;
            child->parent = NULL;
            return;
        }
    }
}

/* Convenience function to detach from current parent without needing to know who it is. */
void SCENE_COMPONENT_DetachFromParent(SceneComponent* sc)
{
    assert(sc != NULL);
    if (sc->parent)
    {
        SCENE_COMPONENT_DetachChild(sc->parent, sc);
	}
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Transform Computation — Dirty Flag Pattern
 *
 *  When any local property changes (position, rotation, scale), the node is
 *  marked dirty. This cascades down to all descendants, because their world
 *  transforms depend on their parent.
 *
 *  ComputeWorldTransform only runs the math if isDirty is true, making it
 *  cheap to call repeatedly on unchanged nodes.
 * ═══════════════════════════════════════════════════════════════════════════ */

 /*------------------------------------------------------------------------------------
  * SCENE_COMPONENT_MarkDirty
  *
  *   Recursively marks this node and all descendants as dirty.
  *   Short-circuits if already dirty (avoids redundant tree traversal).
  *
  *   This is a classic "invalidation cascade" pattern used in scene graphs.
  *------------------------------------------------------------------------------------*/
void SCENE_COMPONENT_MarkDirty(SceneComponent* sc)
{
    assert(sc != NULL);

    if (sc->isDirty) return;    /* Already dirty — subtree is too */

    sc->isDirty = true;

    for (int i = 0; i < sc->childCount; i++)
    {
        SCENE_COMPONENT_MarkDirty(sc->children[i]);
    }
}

/*------------------------------------------------------------------------------------
 * SCENE_COMPONENT_ComputeWorldTransform
 *
 *   Recomputes the local and world transform matrices if the dirty flag is set.
 *   Ensures parent transform is up-to-date first (recursive up the chain).
 *
 *   Local transform construction order (column-major, Raylib convention):
 *       localTransform = Scale × Rotation × Translation
 *
 *   Raylib uses column-major matrices where MatrixMultiply(A, B) = A * B.
 *   So for S*R*T we do: MatrixMultiply(MatrixMultiply(S, R), T)
 *
 *   World transform:
 *       worldTransform = localTransform × parent.worldTransform
 *
 *   This means local transformations are applied first, then the parent,
 *   which is the standard behavior for hierarchical scene graphs.
 *------------------------------------------------------------------------------------*/
void SCENE_COMPONENT_ComputeWorldTransform(SceneComponent* sc)
{
    assert(sc != NULL);

    if (!sc->isDirty) return;

    /* Ensure parent is up-to-date first (recursive) */
    if (sc->parent && sc->parent->isDirty)
    {
        SCENE_COMPONENT_ComputeWorldTransform(sc->parent);
    }

    /* ── Build local transform: S × R × T ── */
    Matrix s = MatrixScale(sc->scale.x, sc->scale.y, sc->scale.z);
    Matrix r = MatrixRotateXYZ(sc->rotation);
    Matrix t = MatrixTranslate(sc->position.x, sc->position.y, sc->position.z);

    sc->localTransform = MatrixMultiply(MatrixMultiply(s, r), t);

    /* ── Combine with parent to get world transform ── */
    if (sc->parent)
    {
        sc->worldTransform = MatrixMultiply(sc->localTransform, sc->parent->worldTransform);
    }
    else
    {
        sc->worldTransform = sc->localTransform;
    }

    sc->isDirty = false;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Direction Vector Extraction
 *
 *  Raylib's Matrix struct stores elements in column-major order:
 *      Column 0 (m0,m1,m2)   = Right axis     (+X local)
 *      Column 1 (m4,m5,m6)   = Up axis         (+Y local)
 *      Column 2 (m8,m9,m10)  = Forward axis    (+Z local, but we use -Z as forward)
 *      Column 3 (m12,m13,m14)= Translation     (world position)
 *
 *  We normalize the extracted vectors to remove any scale component.
 * ═══════════════════════════════════════════════════════════════════════════ */

 /*------------------------------------------------------------------------------------
  * SCENE_COMPONENT_GetForward
  *
  *   Returns the forward direction vector in world space.
  *   Our convention: forward = -Z axis (OpenGL convention).
  *   We negate column 2 of the world matrix and normalize.
  *------------------------------------------------------------------------------------*/
Vector3 SCENE_COMPONENT_GetForward(SceneComponent* sc)
{
    assert(sc != NULL);
    SCENE_COMPONENT_ComputeWorldTransform(sc);

    /* Negate column 2 for -Z forward convention */
    Vector3 fwd = { -sc->worldTransform.m8, 
                    -sc->worldTransform.m9, 
                    -sc->worldTransform.m10 };
    return Vector3Normalize(fwd);
}

/* Right direction = column 0 of world matrix, normalized. */
Vector3 SCENE_COMPONENT_GetRight(SceneComponent* sc)
{
    assert(sc != NULL);
    SCENE_COMPONENT_ComputeWorldTransform(sc);

    Vector3 rgt = { sc->worldTransform.m0, 
                    sc->worldTransform.m1, 
                    sc->worldTransform.m2 };
    return Vector3Normalize(rgt);
}

/* Up direction = column 1 of world matrix, normalized. */
Vector3 SCENE_COMPONENT_GetUp(SceneComponent* sc)
{
    assert(sc != NULL);
    SCENE_COMPONENT_ComputeWorldTransform(sc);

    Vector3 up = { sc->worldTransform.m4, 
                   sc->worldTransform.m5, 
                   sc->worldTransform.m6 };
    return Vector3Normalize(up);
}

/* World position = column 3 (translation) of the world transform matrix. */
Vector3 SCENE_COMPONENT_GetWorldPosition(SceneComponent* sc)
{
    assert(sc != NULL);
    SCENE_COMPONENT_ComputeWorldTransform(sc);

    return (Vector3){ sc->worldTransform.m12, sc->worldTransform.m13, sc->worldTransform.m14 };
}

/* Returns the full world transform matrix. */
Matrix SCENE_COMPONENT_GetWorldTransform(SceneComponent* sc)
{
    assert(sc != NULL);
    SCENE_COMPONENT_ComputeWorldTransform(sc);
	return sc->worldTransform;
}

/* For uniform scale, we can return the X component of the local scale. 
   Non-uniform scaling is not supported in this simple implementation. */
float SCENE_COMPONENT_GetWorldScale(SceneComponent* sc)
{
    return sc->scale.x;
}

/* Get the Actor owner through the base component pointer. */
Actor* SCENE_COMPONENT_GetOwner(SceneComponent* sc)
{
    assert(sc != NULL);
    return sc->base.owner;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Fixed-Timestep Interpolation
 *
 *  Problem:
 *    Physics updates at a fixed rate (e.g., 60Hz) but rendering may run at
 *    a different rate. Without interpolation, objects appear to "stutter"
 *    because their visual position jumps discretely each physics tick.
 *
 *  Solution:
 *    Before rendering, we save the physics state, then interpolate between
 *    the previous and current states using the accumulator fractional alpha.
 *    After rendering, we restore the real physics state.
 *
 *  Frame flow:
 *    1. SavePrevState()              — snapshot before physics ticks
 *    2. [physics ticks run]          — position/rotation updated discretely
 *    3. InterpolateForRender(alpha)  — blend for smooth visual
 *    4. [render]                     — draw at interpolated state
 *    5. RestoreFromInterpolation()   — put real state back for next frame
 *
 *  Reference: Glenn Fiedler "Fix Your Timestep!" article
 * ═══════════════════════════════════════════════════════════════════════════ */

 /* Snapshot the current state as "previous" before physics ticks. */
void SCENE_COMPONENT_SavePrevState(SceneComponent* sc)
{
    assert(sc != NULL);
    sc->prevPosition = sc->position;
    sc->prevRotation = sc->rotation;
}

/*------------------------------------------------------------------------------------
 * SCENE_COMPONENT_InterpolateForRender
 *
 *   Blends between previous and current state for smooth rendering.
 *   alpha = 0.0 → show previous state, alpha = 1.0 → show current state.
 *
 *   Uses Vector3Lerp for linear interpolation.
 *   Note: Euler angle lerp can have issues near ±π (gimbal lock / wrapping),
 *   but works fine for small inter-frame deltas.
 *
 *   The actual (physics-correct) values are saved into prevPosition/prevRotation
 *   so they can be restored after rendering.
 *------------------------------------------------------------------------------------*/
void SCENE_COMPONENT_InterpolateForRender(SceneComponent* sc, float alpha)
{
    assert(sc != NULL);

    /* Save actual physics state */
    Vector3 actualPos = sc->position;
    Vector3 actualRot = sc->rotation;

    /* Interpolate for visual smoothness */
    sc->position = Vector3Lerp(sc->prevPosition, actualPos, alpha);
    sc->rotation = Vector3Lerp(sc->prevRotation, actualRot, alpha);

    /* Store actual state in prev for restore step */
    sc->prevPosition = actualPos;
    sc->prevRotation = actualRot;

    SCENE_COMPONENT_MarkDirty(sc);
}

/* Restore the actual physics state after rendering the interpolated frame. */
void SCENE_COMPONENT_RestoreFromInterpolation(SceneComponent* sc)
{
    assert(sc != NULL);

    sc->position = sc->prevPosition;
    sc->rotation = sc->prevRotation;

    SCENE_COMPONENT_MarkDirty(sc);
}