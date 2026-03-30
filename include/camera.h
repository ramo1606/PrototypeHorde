/**
 * camera.h — Third-Person Orbit Camera (Uncharted-style)
 *
 * The camera orbits the target at a distance, controlled by the player:
 *   - Mouse X  → yaw (horizontal orbit)
 *   - Mouse Y  → pitch (vertical angle, clamped)
 *
 * The orbit angles are INPUT from the player, not derived from movement.
 * Character movement uses the camera yaw as the reference frame:
 *   - W = move away from camera (camera-forward)
 *   - A = move left relative to camera
 *
 * Two modes with smooth transitions:
 *   FOLLOW — default orbit, behind and above
 *   AIM    — over-the-shoulder, closer, lateral offset
 *
 * Camera collision with scenery is stubbed (needs physics world, Fase 2).
 *
 * Usage in a level's Update (fixed tick):
 *   1. Read mouse delta → GAME_CAMERA_RotateByMouse(&cam, dx, dy)
 *   2. Compute move direction relative to camera:
 *        Vector3 forward = GAME_CAMERA_GetForwardXZ(&cam)
 *        Vector3 right   = GAME_CAMERA_GetRightXZ(&cam)
 *        moveDir = forward * inputY + right * inputX
 *   3. Move the character, then:
 *        GAME_CAMERA_SetTarget(&cam, newPos)
 *
 * In the game loop (per visual frame):
 *   4. GAME_CAMERA_Update(&cam, dt)  — smooths + builds Camera3D
 *   5. RENDERER_SetCamera(renderer, cam.camera)
 */

#pragma once

#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>

 /* ── Camera Modes ────────────────────────────────────────────────────────── */

typedef enum CameraMode
{
    CAMERA_MODE_FOLLOW,     /* Default orbit: behind and above              */
    CAMERA_MODE_AIM,        /* Over-the-shoulder: closer, lateral offset    */
} CameraMode;

/* ── Camera Configuration ────────────────────────────────────────────────── */

typedef struct CameraConfig
{
    /* Orbit distances per mode */
    float distFollow;           /* Distance in FOLLOW mode                  */
    float distAim;              /* Distance in AIM mode (closer)            */

    /* Lateral offset per mode (positive = camera shifts right of center) */
    float lateralFollow;        /* Usually 0 (centered behind)              */
    float lateralAim;           /* Positive for over-right-shoulder         */

    /* Vertical */
    float heightFollow;         /* Y offset above target in FOLLOW          */
    float heightAim;            /* Y offset above target in AIM             */

    /* Orbit sensitivity */
    float mouseSensitivity;     /* Degrees per pixel of mouse movement      */

    /* Pitch limits (degrees) */
    float pitchMin;             /* Lowest angle (looking up, negative)      */
    float pitchMax;             /* Highest angle (looking down, positive)   */

    /* Smoothing */
    float smoothSpeed;          /* Exponential smooth factor (higher=snappier) */
    float transitionDuration;   /* Seconds for FOLLOW↔AIM mode blend        */

    /* Look-at offset */
    float lookAtHeight;         /* Y offset on look-at point (chest height) */
} CameraConfig;

/* ── GameCamera Struct ───────────────────────────────────────────────────── */

typedef struct GameCamera
{
    /* Output: raylib camera, ready for the renderer */
    Camera3D camera;

    /* Configuration */
    CameraConfig config;

    /* Orbit state (player-controlled) */
    float yaw;                  /* Horizontal orbit angle (radians)         */
    float pitch;                /* Vertical orbit angle (radians)           */

    /* Mode */
    CameraMode mode;
    CameraMode prevMode;

    /* Target tracking */
    Vector3 targetPos;          /* World position to orbit around           */

    /* Smoothed state */
    Vector3 currentPos;         /* Smoothed camera world position           */
    Vector3 currentLookAt;      /* Smoothed look-at point                   */

    /* Mode transition */
    bool  transitioning;
    float transitionTimer;
    float distFrom, distTo;
    float lateralFrom, lateralTo;
    float heightFrom, heightTo;
} GameCamera;

/* ── Lifecycle ───────────────────────────────────────────────────────────── */

void GAME_CAMERA_Init(GameCamera* cam);

/* ── Per-Frame Update ────────────────────────────────────────────────────── */

void GAME_CAMERA_Update(GameCamera* cam, float dt);

/* ── Orbit Control ───────────────────────────────────────────────────────── */

void GAME_CAMERA_RotateByMouse(GameCamera* cam, float dx, float dy);

/* ── Target Tracking ─────────────────────────────────────────────────────── */

void GAME_CAMERA_SetTarget(GameCamera* cam, Vector3 pos);

/* ── Direction Queries (for camera-relative movement) ────────────────────── */

Vector3 GAME_CAMERA_GetForwardXZ(const GameCamera* cam);
Vector3 GAME_CAMERA_GetRightXZ(const GameCamera* cam);

/* ── Mode Control ────────────────────────────────────────────────────────── */

void GAME_CAMERA_SetMode(GameCamera* cam, CameraMode mode);

/* ── Configuration ───────────────────────────────────────────────────────── */

void GAME_CAMERA_SetConfig(GameCamera* cam, CameraConfig config);