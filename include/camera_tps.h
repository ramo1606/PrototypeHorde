/*******************************************************************************************
*
*   camera_tps.h — Third-Person Spring Camera
*
*   CameraTPS implements a classic third-person camera that follows the player
*   using a critically-damped spring system. The camera smoothly tracks the player
*   and automatically avoids clipping through walls via raycasting.
*
*   Architecture:
*       CameraComponent (base)
*           └── CameraTPS
*                   ├── Spring physics (position smoothing)
*                   ├── Orbit angles (yaw/pitch, mouse-controlled)
*                   ├── Ideal position (behind + above player via orbit)
*                   └── Wall clamp (ray cast collision avoidance)
*
*   Orbit Mode (Uncharted / BotW style):
*       The camera maintains its own yaw and pitch angles, independent of the
*       actor's facing direction. The player controls these via mouse/right stick.
*       The camera orbits around the actor using spherical coordinates:
*
*           idealPos.x = playerPos.x - cos(pitch) * sin(yaw) * horzDist
*           idealPos.y = playerPos.y + sin(pitch) * horzDist + vertDist
*           idealPos.z = playerPos.z - cos(pitch) * cos(yaw) * horzDist
*
*       Pitch is clamped to avoid flipping (default: -75° to +75°).
*
*       The camera exposes its planar forward/right vectors so that the
*       movement system can transform input relative to the camera's view.
*
*   Naming Convention:
*       API:     CAMERA_TPS_*
*
********************************************************************************************/
#pragma once

#include "camera_component.h"
#include "raymath.h"

typedef struct CameraTPS CameraTPS;
typedef struct Actor Actor;

/* ── Camera TPS Struct ───────────────────────────────────────────────────── */
struct CameraTPS
{
    CameraComponent base;       /* Inherited camera component (must be first field) */

    Vector3 actualPos;          /* Current smoothed camera position in world space   */
    Vector3 velocity;           /* Current spring velocity (used by damped spring)   */

    float horzDist;             /* Horizontal distance behind the player             */
    float vertDist;             /* Vertical distance above the player                */
    float targetDist;           /* Distance forward from player to look-at point     */
    float springConstant;       /* Spring stiffness (higher = snappier follow)       */

    /* ── Orbit state (camera-owned orientation) ──────────────────────────── */
    float yaw;                  /* Horizontal orbit angle in radians (around Y axis) */
    float pitch;                /* Vertical orbit angle in radians (up/down tilt)    */
    float sensitivity;          /* Mouse/stick sensitivity multiplier                */
    float pitchMin;             /* Lower pitch clamp (radians, negative = look up)   */
    float pitchMax;             /* Upper pitch clamp (radians, positive = look down)  */
};

/* ── Public API ──────────────────────────────────────────────────────────── */
CameraTPS* CAMERA_TPS_Create(Actor* owner);
void CAMERA_TPS_SnapToIdeal(CameraTPS* ctps);

void CAMERA_TPS_SetDistances(CameraTPS* ctps, float horz, float vert, float target);
void CAMERA_TPS_SetSpring(CameraTPS* ctps, float springConstant);

/* ── Orbit Control ───────────────────────────────────────────────────────── */

/* Apply mouse/stick delta to orbit angles. deltaYaw and deltaPitch are raw
 * input deltas (pixels or stick values) — sensitivity is applied internally. */
void CAMERA_TPS_RotateOrbit(CameraTPS* ctps, float deltaYaw, float deltaPitch);

/* Set orbit angles directly (radians). Useful for snapping to a specific view. */
void CAMERA_TPS_SetOrbitAngles(CameraTPS* ctps, float yaw, float pitch);

/* Set sensitivity and pitch clamp range. */
void CAMERA_TPS_SetOrbitParams(CameraTPS* ctps, float sensitivity, float pitchMin, float pitchMax);

/* ── Camera Direction Queries (for movement system) ──────────────────────── */

/* Returns the camera's forward direction projected onto the XZ plane,
 * normalized. Y component is always 0. Used by the movement system to
 * transform WASD input into world-space direction. */
Vector3 CAMERA_TPS_GetPlanarForward(CameraTPS* ctps);

/* Returns the camera's right direction projected onto the XZ plane,
 * normalized. Y component is always 0. */
Vector3 CAMERA_TPS_GetPlanarRight(CameraTPS* ctps);