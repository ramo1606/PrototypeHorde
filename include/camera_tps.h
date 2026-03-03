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
*                   ├── Ideal position (behind + above player)
*                   └── Wall clamp (ray cast collision avoidance)
*
*   The camera position is computed as:
*       idealPos = playerPos - forward * horzDist + up * vertDist
*       target   = playerPos + forward * targetDist
*
*   A spring-damper system smooths the transition from actual to ideal position,
*   preventing jerky movement. Wall clamping prevents the camera from going
*   behind geometry by casting a ray from target to camera position.
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
};

/* ── Public API ──────────────────────────────────────────────────────────── */
CameraTPS* CAMERA_TPS_Create(Actor* owner);
void CAMERA_TPS_SnapToIdeal(CameraTPS* ctps);

void CAMERA_TPS_SetDistances(CameraTPS* ctps, float horz, float vert, float target);
void CAMERA_TPS_SetSpring(CameraTPS* ctps, float springConstant);