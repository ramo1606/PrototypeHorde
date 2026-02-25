#pragma once

/*
 * camera_tps.h — Third-person spring-damper follow camera.
 *
 * CameraTPS extends CameraComponent by embedding it as the first field.
 * Each fixed tick it runs a critically-damped spring simulation to smoothly
 * chase an "ideal" position behind and above the target actor, then clamps
 * the result against world geometry via a raycast to prevent the camera
 * from clipping through walls.
 *
 * Spring formula (critically damped):
 *   dampening = 2 * sqrt(springConstant)
 *   accel     = -springConstant * (actualPos - idealPos) - dampening * velocity
 *   velocity += accel * dt
 *   actualPos += velocity * dt
 *
 * Reference: "Game Programming in C++" by Sanjay Madhav, Chapter 9.
 *
 * Architecture position:
 *   Actor → CameraTPS (embeds CameraComponent → SceneComponent → Component)
 */

#include "camera_component.h"
#include "raymath.h"

typedef struct CameraTPS CameraTPS;
typedef struct Actor Actor;

/* ── CameraTPS Struct ───────────────────────────────────────────── */

struct CameraTPS
{
    CameraComponent base;       /* Embedded CameraComponent — must be first field for safe casting */

    Vector3 actualPos;          /* Current smoothed camera position driven by the spring simulation */
    Vector3 velocity;           /* Current spring velocity (world units per second) */

    float horzDist;             /* Ideal horizontal distance behind the target (along -forward) */
    float vertDist;             /* Ideal vertical offset above the target's position */
    float targetDist;           /* Distance in front of the actor that the camera looks at */
    float springConstant;       /* Spring stiffness k; critically-damped when dampening = 2*sqrt(k) */
};

/* ── Public API ─────────────────────────────────────────────────── */

CameraTPS* CAMERA_TPS_Create(Actor* owner);          // Allocate a CameraTPS, initialise with default distances/spring, and snap to ideal position
void       CAMERA_TPS_SnapToIdeal(CameraTPS* ctps);  // Teleport camera to the ideal position and zero velocity (call after a level transition)

void CAMERA_TPS_SetDistances(CameraTPS* ctps, float horz, float vert, float target); // Override the horizontal, vertical, and look-at distances
void CAMERA_TPS_SetSpring(CameraTPS* ctps, float springConstant);                    // Override the spring stiffness constant