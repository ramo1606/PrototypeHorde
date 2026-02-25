#pragma once

/*
 * move_component.h — Simple kinematic movement component.
 *
 * MoveComponent extends Component (no transform node of its own) and moves
 * the owning Actor each fixed tick by applying forward, angular (yaw), and
 * strafe speeds.  It is the minimal locomotion building block — a higher-
 * level character controller or AI would set these speeds each frame.
 *
 * Architecture position:
 *   Actor → MoveComponent (updateOrder 10, updates early so position is
 *   ready before camera and collider components run)
 */

#include "component.h"

typedef struct MoveComponent MoveComponent;

/* ── MoveComponent Struct ───────────────────────────────────────── */

struct MoveComponent
{
    Component base;        /* Embedded Component — must be first field for safe casting */
    float angularSpeed;    /* Yaw rotation speed in radians per second (positive = turn left) */
    float forwardSpeed;    /* Movement speed along the actor's forward axis in units per second */
    float strafeSpeed;     /* Movement speed along the actor's right axis in units per second */
};

/* ── Public API ─────────────────────────────────────────────────── */

MoveComponent* MOVE_COMPONENT_Create(Actor* owner);                                           // Allocate a MoveComponent, initialise it with zero speeds, and attach it to the owner
void           MOVE_COMPONENT_SetSpeeds(MoveComponent* mc, float forward, float angular, float strafe); // Set all three movement speeds at once