#pragma once
#include "component.h"
#include "raylib.h"
#include <stdbool.h>

typedef struct Actor Actor;

typedef struct SphereComponent
{
    Component base;
    Vector3 offset;
    float radius;
    Vector3 worldCenter;
    float worldRadius;
    unsigned int layerMask;
    bool isTrigger;
} SphereComponent;

SphereComponent* SPHERE_COMPONENT_Create(Actor* owner);
void SPHERE_COMPONENT_Set(SphereComponent* sc, Vector3 offset, float radius);

Vector3 SPHERE_COMPONENT_GetWorldCenter(SphereComponent* sc);
float SPHERE_COMPONENT_GetWorldRadius(SphereComponent* sc);

void SPHERE_COMPONENT_DrawWires(SphereComponent* sc, Color color);