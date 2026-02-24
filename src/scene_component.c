#include "scene_component.h"
#include "actor.h"
#include <stddef.h>
#include <assert.h>

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

void SCENE_COMPONENT_InitRoot(SceneComponent* sc, Actor* owner)
{
    assert(sc != NULL);
    assert(owner != NULL);

    sc->base.owner       = owner;
    sc->base.type        = COMPONENT_SCENE;
    sc->base.updateOrder = 0;
    sc->base.Update      = NULL;
    sc->base.Input       = NULL;
    sc->base.Destroy     = NULL;

    SCENE_COMPONENT_InitFields(sc);
}

void SCENE_COMPONENT_Init(SceneComponent* sc, Actor* owner, ComponentType type, int updateOrder)
{
    assert(sc != NULL);
    assert(owner != NULL);

    COMPONENT_Init(&sc->base, owner, type, updateOrder);
    SCENE_COMPONENT_InitFields(sc);
}

void SCENE_COMPONENT_AttachChild(SceneComponent* parent, SceneComponent* child)
{
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
    assert(sc != NULL);
    SCENE_COMPONENT_ComputeWorldTransform(sc);

    Vector3 fwd = { -sc->worldTransform.m8, 
                    -sc->worldTransform.m9, 
                    -sc->worldTransform.m10 };
    return Vector3Normalize(fwd);
}

Vector3 SCENE_COMPONENT_GetRight(SceneComponent* sc)
{
    assert(sc != NULL);
    SCENE_COMPONENT_ComputeWorldTransform(sc);

    Vector3 rgt = { sc->worldTransform.m0, 
                    sc->worldTransform.m1, 
                    sc->worldTransform.m2 };
    return Vector3Normalize(rgt);
}

Vector3 SCENE_COMPONENT_GetUp(SceneComponent* sc)
{
    assert(sc != NULL);
    SCENE_COMPONENT_ComputeWorldTransform(sc);

    Vector3 up = { sc->worldTransform.m4, 
                   sc->worldTransform.m5, 
                   sc->worldTransform.m6 };
    return Vector3Normalize(up);
}

Vector3 SCENE_COMPONENT_GetWorldPosition(SceneComponent* sc)
{
    assert(sc != NULL);
    SCENE_COMPONENT_ComputeWorldTransform(sc);

    return (Vector3){ sc->worldTransform.m12, sc->worldTransform.m13, sc->worldTransform.m14 };
}

Actor* SCENE_COMPONENT_GetOwner(SceneComponent* sc)
{
    assert(sc != NULL);
    return sc->base.owner;
}

void SCENE_COMPONENT_SavePrevState(SceneComponent* sc)
{
    assert(sc != NULL);
    sc->prevPosition = sc->position;
    sc->prevRotation = sc->rotation;
}

void SCENE_COMPONENT_InterpolateForRender(SceneComponent* sc, float alpha)
{
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
    assert(sc != NULL);

    sc->position = sc->prevPosition;
    sc->rotation = sc->prevRotation;

    SCENE_COMPONENT_MarkDirty(sc);
}