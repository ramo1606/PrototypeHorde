#pragma once
#include "component.h"

typedef struct 
{
    Component base;
    float angularSpeed;
    float forwardSpeed;
    float strafeSpeed;
} MoveComponent;

MoveComponent* MOVE_COMPONENT_Create(Actor* owner);