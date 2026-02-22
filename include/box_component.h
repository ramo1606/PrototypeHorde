#pragma once

#include "component.h"
#include "raylib.h"
#include <stdbool.h>

typedef struct Actor Actor;

typedef struct BoxComponent
{
    Component   component;
    BoundingBox objectBox;  /* Local-space AABB */
    BoundingBox worldBox;   /* World-space AABB (recomputed each frame) */
} BoxComponent;

BoxComponent* BOX_COMPONENT_Create(Actor* owner);
void BOX_COMPONENT_SetObjectBox(BoxComponent* bc, BoundingBox objectBox);
BoundingBox BOX_COMPONENT_GetWorldBox(BoxComponent* bc);

/* Debug visualization */
void BOX_COMPONENT_DrawWorldBox(BoxComponent* bc, Color color);