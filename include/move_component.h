#pragma once
#include "component.h"

typedef struct MoveComponent MoveComponent;

struct MoveComponent
{
    Component base;
    float angularSpeed;
    float forwardSpeed;
    float strafeSpeed;
};

MoveComponent* MOVE_COMPONENT_Create(Actor* owner);