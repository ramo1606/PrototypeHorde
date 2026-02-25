#include "scene_component.h"
#include "actor.h"
#include <stddef.h>
#include <assert.h>

static void SCENE_COMPONENT_InitFields(SceneComponent* sc)
{
    /*
     * Reset all transform data to a neutral state:
     *   - position/rotation = zero vectors
     *   - scale = (1,1,1) — identity, no scaling
     *   - prev state = zero (snapshot used for visual interpolation)
     *   - local/world transforms = identity matrices
     *   - isDirty = true so the first ComputeWorldTransform call runs
     *   - parent = NULL, no children
     */
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

void SCENE_COMPONENT_InitRoot(SceneComponent* sc, Actor* owner)
{
    /*
     * Initialise the actor's root transform node.  Unlike regular
     * components the root is NOT inserted into the actor's component list
     * (it exists outside the sorted update array) so we set the base
     * fields manually instead of calling COMPONENT_Init.
     */
    assert(sc != NULL);
    assert(owner != NULL);

    sc->base.owner       = owner;
    sc->base.type        = COMPONENT_TYPE_SCENE;
    sc->base.updateOrder = 0;
    sc->base.Update      = NULL;
    sc->base.Input       = NULL;
    sc->base.Destroy     = NULL;

    SCENE_COMPONENT_InitFields(sc);
}

void SCENE_COMPONENT_Init(SceneComponent* sc, Actor* owner, ComponentType type, int updateOrder)
{
    /*
     * Initialise a scene component that participates in the normal
     * component update list.  Calls COMPONENT_Init (which inserts it into
     * the owner's sorted array) then resets the transform fields.
     */
    assert(sc != NULL);
    assert(owner != NULL);

    COMPONENT_Init(&sc->base, owner, type, updateOrder);
    SCENE_COMPONENT_InitFields(sc);
}

void SCENE_COMPONENT_AttachChild(SceneComponent* parent, SceneComponent* child)
{
    /*
     * Re-parent child to parent.  If child already has a parent it is
     * detached first to avoid dangling entries.  The child's dirty flag is
     * set so its world transform is recomputed on the next access.
     */
    assert(parent != NULL);
    assert(child != NULL);

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

void SCENE_COMPONENT_DetachChild(SceneComponent* parent, SceneComponent* child)
{
    /*
     * Swap-remove the child from the parent's children array (O(n) scan,
     * O(1) remove).  Clears child->parent to signal it is now a root node.
     */
    assert(parent != NULL);
    assert(child != NULL);

    for (int i = 0; i < parent->childCount; i++)
    {
        if (parent->children[i] == child)
        {
            parent->children[i] = parent->children[parent->childCount - 1];
            parent->children[parent->childCount - 1] = NULL;
            parent->childCount--;
            child->parent = NULL;
            return;
        }
    }
}

void SCENE_COMPONENT_MarkDirty(SceneComponent* sc)
{
    /*
     * Dirty-flag pattern: propagate the dirty mark down the subtree so
     * that all descendants know their world transforms are stale.  Early-
     * exit if already dirty to avoid redundant recursion.
     */
    assert(sc != NULL);

    if (sc->isDirty) return;

    sc->isDirty = true;

    for (int i = 0; i < sc->childCount; i++)
    {
        SCENE_COMPONENT_MarkDirty(sc->children[i]);
    }
}

void SCENE_COMPONENT_ComputeWorldTransform(SceneComponent* sc)
{
    /*
     * Lazy world-transform recomputation (dirty-flag pattern).
     *
     * Build order:
     *   localTransform = T * R * S  (scale, then rotate, then translate)
     *
     * The Raylib matrix layout is column-major, so MatrixMultiply(S, R)
     * applies S first, R second.  Adding T last gives the standard SRT
     * decomposition used for hierarchical transforms.
     *
     * If this node has a parent that is also dirty, recurse upward so the
     * parent's world transform is fresh before multiplying.
     *
     *   worldTransform = localTransform * parent->worldTransform
     *
     * Reference: "Game Programming in C++" (Madhav), transform hierarchy
     * chapter.
     */
    assert(sc != NULL);

    if (!sc->isDirty) return;

    if (sc->parent && sc->parent->isDirty)
    {
        SCENE_COMPONENT_ComputeWorldTransform(sc->parent);
    }

    Matrix s = MatrixScale(sc->scale.x, sc->scale.y, sc->scale.z);
    Matrix r = MatrixRotateXYZ(sc->rotation);
    Matrix t = MatrixTranslate(sc->position.x, sc->position.y, sc->position.z);

    sc->localTransform = MatrixMultiply(MatrixMultiply(s, r), t);

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

Vector3 SCENE_COMPONENT_GetForward(SceneComponent* sc)
{
    /*
     * In Raylib's column-major layout:
     *   Column 0 (m0,m1,m2)   = local X (right)
     *   Column 1 (m4,m5,m6)   = local Y (up)
     *   Column 2 (m8,m9,m10)  = local Z (back in OpenGL convention)
     *
     * Forward is -Z, so negate column 2.
     */
    assert(sc != NULL);
    SCENE_COMPONENT_ComputeWorldTransform(sc);

    Vector3 fwd = { -sc->worldTransform.m8, 
                    -sc->worldTransform.m9, 
                    -sc->worldTransform.m10 };
    return Vector3Normalize(fwd);
}

Vector3 SCENE_COMPONENT_GetRight(SceneComponent* sc)
{
    /*
     * Right is +X: extract column 0 of the world transform matrix.
     */
    assert(sc != NULL);
    SCENE_COMPONENT_ComputeWorldTransform(sc);

    Vector3 rgt = { sc->worldTransform.m0, 
                    sc->worldTransform.m1, 
                    sc->worldTransform.m2 };
    return Vector3Normalize(rgt);
}

Vector3 SCENE_COMPONENT_GetUp(SceneComponent* sc)
{
    /*
     * Up is +Y: extract column 1 of the world transform matrix.
     */
    assert(sc != NULL);
    SCENE_COMPONENT_ComputeWorldTransform(sc);

    Vector3 up = { sc->worldTransform.m4, 
                   sc->worldTransform.m5, 
                   sc->worldTransform.m6 };
    return Vector3Normalize(up);
}

Vector3 SCENE_COMPONENT_GetWorldPosition(SceneComponent* sc)
{
    /*
     * The translation is stored in column 3 of the 4×4 matrix:
     *   (m12, m13, m14) in Raylib's column-major layout.
     */
    assert(sc != NULL);
    SCENE_COMPONENT_ComputeWorldTransform(sc);

    return (Vector3){ sc->worldTransform.m12, sc->worldTransform.m13, sc->worldTransform.m14 };
}

Actor* SCENE_COMPONENT_GetOwner(SceneComponent* sc)
{
    /*
     * Delegate to the embedded base component which holds the owner pointer.
     */
    assert(sc != NULL);
    return sc->base.owner;
}

void SCENE_COMPONENT_SavePrevState(SceneComponent* sc)
{
    /*
     * Snapshot the current position and rotation before the fixed-timestep
     * update so that SCENE_COMPONENT_InterpolateForRender can lerp between
     * the previous and new state for sub-frame visual smoothness.
     * Called once per frame, before all fixed updates.
     */
    assert(sc != NULL);
    sc->prevPosition = sc->position;
    sc->prevRotation = sc->rotation;
}

void SCENE_COMPONENT_InterpolateForRender(SceneComponent* sc, float alpha)
{
    /*
     * Visual interpolation for the fixed-timestep loop.
     *
     * alpha = accumulator / FIXED_TIMESTEP  (range [0, 1))
     *
     * We lerp position and rotation between the saved previous state and
     * the current physics state.  The "actual" values are temporarily
     * stashed in prevPosition/prevRotation so that
     * SCENE_COMPONENT_RestoreFromInterpolation can put them back after the
     * render pass without a second snapshot.
     *
     * This pattern gives smooth rendering even when physics runs at a lower
     * frequency than the display refresh rate.
     */
    assert(sc != NULL);

    Vector3 actualPos = sc->position;
    Vector3 actualRot = sc->rotation;

    sc->position = Vector3Lerp(sc->prevPosition, actualPos, alpha);
    sc->rotation = Vector3Lerp(sc->prevRotation, actualRot, alpha);

    sc->prevPosition = actualPos;
    sc->prevRotation = actualRot;

    SCENE_COMPONENT_MarkDirty(sc);
}

void SCENE_COMPONENT_RestoreFromInterpolation(SceneComponent* sc)
{
    /*
     * After rendering, restore the true physics position/rotation from the
     * values we stashed in prevPosition/prevRotation during
     * InterpolateForRender.  This keeps the physics state correct for the
     * next fixed update.
     */
    assert(sc != NULL);

    sc->position = sc->prevPosition;
    sc->rotation = sc->prevRotation;

    SCENE_COMPONENT_MarkDirty(sc);
}