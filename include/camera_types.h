#pragma once

#include "raylib.h"
#include <stdbool.h>

/* Isometric camera with low-angle perspective.
 *
 * Fixed elevation + azimuth — no rotation, no aim mode. The camera
 * follows a target with frame-rate independent exponential smoothing.
 * Multiplayer mode is centroid-aware (target = group centroid;
 * distance scales with spread). The multiplayer fields are present
 * but unused in singleplayer.
 */

typedef struct CameraConfig
{
    /* Fixed view angles (degrees). */
    float elevationDeg;     /* Pitch downward; classic iso ≈ 30°.    */
    float azimuthDeg;       /* Rotation around Y; classic iso = 45°. */

    /* Projection. */
    float fovy;             /* Perspective field of view (degrees).  */

    /* Single-target distance (singleplayer). */
    float distance;

    /* Look-at point lift above the target's feet. */
    float lookAtHeight;

    /* Smoothing. Higher = snappier. Frame-rate independent. */
    float smoothSpeed;

    /* Multiplayer: distance scales linearly with the group's spread.
     * Unused while only a single target is set. */
    float multiplayerMinDistance;
    float multiplayerMaxDistance;
    float multiplayerSpreadFactor;
} CameraConfig;

typedef struct GameCamera
{
    /* Output: raylib camera, pushed to RendererBuildDrawList. */
    Camera3D camera;

    CameraConfig config;

    /* Target tracking. In singleplayer: the player's world position.
     * In multiplayer: centroid of live players. */
    Vector3 targetPos;

    /* Multiplayer spread (max pairwise distance). 0 in singleplayer. */
    float spread;

    /* Smoothed look-at point (lerps toward targetPos + lookAtHeight). */
    Vector3 currentLookAt;
} GameCamera;
