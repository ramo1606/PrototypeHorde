#pragma once
#include "component.h"
#include "raylib.h"
#include <stdbool.h>

typedef struct Actor Actor;

typedef struct SphereComponent
{
    Component component;
    Vector3   offset;       /* Local-space offset from actor origin */
    float     radius;       /* Sphere radius (local space) */
    Vector3   worldCenter;  /* Recomputed each frame */
    float     worldRadius;  /* Recomputed each frame */
} SphereComponent;

SphereComponent* SPHERE_COMPONENT_Create(Actor* owner);
void SPHERE_COMPONENT_Set(SphereComponent* sc, Vector3 offset, float radius);
Vector3 SPHERE_COMPONENT_GetWorldCenter(SphereComponent* sc);
float   SPHERE_COMPONENT_GetRadius(SphereComponent* sc);

/* Debug visualization */
void SPHERE_COMPONENT_DrawWires(SphereComponent* sc, Color color);