#pragma once

#include "raylib.h"
#include <stdbool.h>

typedef enum
{
    CAMERA_MODE_FOLLOW,     /* Default orbit: behind and above              */
    CAMERA_MODE_AIM,        /* Over-the-shoulder: closer, lateral offset    */
} CameraMode;

typedef struct CameraConfig
{
    /* Orbit distances per mode */
    float distFollow;
    float distAim;

    /* Lateral offset per mode (positive = camera shifts right of center) */
    float lateralFollow;
    float lateralAim;

    /* Vertical offsets */
    float heightFollow;
    float heightAim;

    /* Orbit sensitivity */
    float mouseSensitivity;     /* Degrees per pixel of mouse movement */

    /* Pitch limits (degrees) */
    float pitchMin;
    float pitchMax;

    /* Smoothing */
    float smoothSpeed;          /* Exponential smooth factor (higher = snappier) */
    float transitionDuration;   /* Seconds for FOLLOW↔AIM mode blend */

    /* Look-at offset */
    float lookAtHeight;         /* Y offset on look-at point (chest height) */
} CameraConfig;

typedef struct GameCamera
{
    /* Output: raylib camera, ready for the renderer */
    Camera3D camera;

    CameraConfig config;

    /* Orbit state (player-controlled) */
    float yaw;                  /* radians */
    float pitch;                /* radians */

    CameraMode mode;
    CameraMode prevMode;

    Vector3 targetPos;          /* World position to orbit around */

    /* Smoothed state */
    Vector3 currentPos;
    Vector3 currentLookAt;

    /* Mode transition */
    bool  transitioning;
    float transitionTimer;
    float distFrom, distTo;
    float lateralFrom, lateralTo;
    float heightFrom, heightTo;
} GameCamera;
