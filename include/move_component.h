/*******************************************************************************************
*
*   move_component.h — Move Component (Basic Linear/Angular Movement)
*
*   MoveComponent provides simple movement by applying forward speed, angular speed
*   (yaw rotation), and strafe speed each tick. It reads the actor's current
*   orientation to determine movement directions.
*
*   This is a basic movement system suitable for simple actors (spinning props,
*   patrolling enemies). Player characters typically use a more advanced
*   CharacterMovementComponent.
*
*   Movement is applied relative to the actor's local axes:
*       Forward = along the actor's -Z forward vector × forwardSpeed
*       Strafe  = along the actor's +X right vector × strafeSpeed
*       Angular = yaw rotation around Y axis × angularSpeed
*
*   Naming Convention:
*       API:     MOVE_COMPONENT_*
*
********************************************************************************************/
#pragma once
#include "component.h"

typedef struct MoveComponent MoveComponent;

/* ── Move Component Struct ───────────────────────────────────────────────── */
struct MoveComponent
{
    Component base;             /* Inherited base component (must be first field)     */
    float angularSpeed;         /* Yaw rotation speed in radians/second               */
    float forwardSpeed;         /* Movement speed along forward vector (units/second) */
    float strafeSpeed;          /* Movement speed along right vector (units/second)   */
};

/* ── Public API ──────────────────────────────────────────────────────────── */
MoveComponent* MOVE_COMPONENT_Create(Actor* owner);
void MOVE_COMPONENT_SetSpeeds(MoveComponent* mc, float forward, float angular, float strafe);