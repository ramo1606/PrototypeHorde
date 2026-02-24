#pragma once

#include "component.h"
#include "raylib.h"
#include <stdbool.h>

typedef struct BoxComponent BoxComponent;
typedef struct Actor Actor;

struct BoxComponent
{
    Component   base;
    BoundingBox objectBox;
    BoundingBox worldBox;
    unsigned int layerMask;         /* Phase 5: collision layer */
    bool isTrigger;                 /* Phase 5: overlap only, no blocking */
};

BoxComponent* BOX_COMPONENT_Create(Actor* owner);
void BOX_COMPONENT_SetObjectBox(BoxComponent* bc, BoundingBox objectBox);
void BOX_COMPONENT_SetFromMesh(BoxComponent* bc, Mesh mesh);

BoundingBox BOX_COMPONENT_GetWorldBox(BoxComponent* bc);
void BOX_COMPONENT_DrawWorldBox(BoxComponent* bc, Color color);