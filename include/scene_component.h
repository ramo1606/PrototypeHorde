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

    Vector3 position;
    Vector3 rotation;
    Vector3 scale;

    Vector3 prevPosition;
    Vector3 prevRotation;

    Matrix  localTransform;
    Matrix  worldTransform;
    bool    isDirty;

    SceneComponent* parent;
    SceneComponent* children[SCENE_MAX_CHILDREN];
    int             childCount;
};

void SCENE_COMPONENT_InitRoot(SceneComponent* sc, Actor* owner);
void SCENE_COMPONENT_Init(SceneComponent* sc, Actor* owner, ComponentType type, int updateOrder);

void SCENE_COMPONENT_AttachChild(SceneComponent* parent, SceneComponent* child);
void SCENE_COMPONENT_DetachChild(SceneComponent* parent, SceneComponent* child);
void SCENE_COMPONENT_DetachFromParent(SceneComponent* sc);

void SCENE_COMPONENT_MarkDirty(SceneComponent* sc);
void SCENE_COMPONENT_ComputeWorldTransform(SceneComponent* sc);

Vector3 SCENE_COMPONENT_GetForward(SceneComponent* sc);
Vector3 SCENE_COMPONENT_GetRight(SceneComponent* sc);
Vector3 SCENE_COMPONENT_GetUp(SceneComponent* sc);
Vector3 SCENE_COMPONENT_GetWorldPosition(SceneComponent* sc);
Matrix  SCENE_COMPONENT_GetWorldTransform(SceneComponent* sc);
float   SCENE_COMPONENT_GetWorldScale(SceneComponent* sc);

Actor* SCENE_COMPONENT_GetOwner(SceneComponent* sc);

void SCENE_COMPONENT_SavePrevState(SceneComponent* sc);
void SCENE_COMPONENT_InterpolateForRender(SceneComponent* sc, float alpha);
void SCENE_COMPONENT_RestoreFromInterpolation(SceneComponent* sc);