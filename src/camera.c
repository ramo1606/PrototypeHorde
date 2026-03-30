#include "camera.h"
#include "reasings.h"

#include <math.h>

/*******************************************************************************************
*
*   camera.c — Third-Person Orbit Camera (Uncharted-style)
*
*   The camera sits on a sphere around the target. Yaw and pitch are
*   controlled by the mouse. The character moves relative to the camera's
*   facing direction, not in world space.
*
*   Spherical-to-cartesian conversion:
*     x = distance * cos(pitch) * sin(yaw)
*     y = distance * sin(pitch)
*     z = distance * cos(pitch) * cos(yaw)
*
*   Then we add the lateral offset (perpendicular to the orbit direction
*   in the XZ plane) and the height offset.
*
********************************************************************************************/

/* ═══════════════════════════════════════════════════════════════════════════
 *  Default Configuration
 * ═══════════════════════════════════════════════════════════════════════════ */

static const CameraConfig DEFAULT_CONFIG = {
    .distFollow = 10.0f,
    .distAim = 5.0f,
    .lateralFollow = 0.0f,
    .lateralAim = 1.5f,
    .heightFollow = 4.0f,
    .heightAim = 2.5f,
    .mouseSensitivity = 0.15f,
    .pitchMin = -30.0f,
    .pitchMax = 60.0f,
    .smoothSpeed = 10.0f,
    .transitionDuration = 0.3f,
    .lookAtHeight = 1.2f,
};

/* ═══════════════════════════════════════════════════════════════════════════
 *  Helpers
 * ═══════════════════════════════════════════════════════════════════════════ */

 /* Get current mode parameters, handling transitions with easing */
static void GetCurrentParams(const GameCamera* cam,
    float* outDist, float* outLateral, float* outHeight)
{
    if (cam->transitioning)
    {
        float t = cam->transitionTimer;
        float d = cam->config.transitionDuration;

        *outDist = EaseSineInOut(t, cam->distFrom, cam->distTo - cam->distFrom, d);
        *outLateral = EaseSineInOut(t, cam->lateralFrom, cam->lateralTo - cam->lateralFrom, d);
        *outHeight = EaseSineInOut(t, cam->heightFrom, cam->heightTo - cam->heightFrom, d);
    }
    else
    {
        switch (cam->mode)
        {
        case CAMERA_MODE_AIM:
            *outDist = cam->config.distAim;
            *outLateral = cam->config.lateralAim;
            *outHeight = cam->config.heightAim;
            break;
        case CAMERA_MODE_FOLLOW: /* fall through */
        default:
            *outDist = cam->config.distFollow;
            *outLateral = cam->config.lateralFollow;
            *outHeight = cam->config.heightFollow;
            break;
        }
    }
}

static void GetParamsForMode(const GameCamera* cam, CameraMode mode,
    float* outDist, float* outLateral, float* outHeight)
{
    switch (mode)
    {
    case CAMERA_MODE_AIM:
        *outDist = cam->config.distAim;
        *outLateral = cam->config.lateralAim;
        *outHeight = cam->config.heightAim;
        break;
    case CAMERA_MODE_FOLLOW: /* fall through */
    default:
        *outDist = cam->config.distFollow;
        *outLateral = cam->config.lateralFollow;
        *outHeight = cam->config.heightFollow;
        break;
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Lifecycle
 * ═══════════════════════════════════════════════════════════════════════════ */

void GAME_CAMERA_Init(GameCamera* cam)
{
    cam->config = DEFAULT_CONFIG;
    cam->mode = CAMERA_MODE_FOLLOW;
    cam->prevMode = CAMERA_MODE_FOLLOW;

    /* Start looking from behind (+Z direction, yaw = 0) */
    cam->yaw = 0.0f;
    cam->pitch = 20.0f * DEG2RAD;   /* Slight downward angle */

    cam->targetPos = (Vector3){ 0.0f, 0.0f, 0.0f };

    cam->transitioning = false;
    cam->transitionTimer = 0.0f;

    /* Compute initial position */
    float cosPitch = cosf(cam->pitch);
    float sinPitch = sinf(cam->pitch);
    float dist = cam->config.distFollow;

    cam->currentPos = (Vector3){
        cam->targetPos.x + dist * cosPitch * sinf(cam->yaw),
        cam->targetPos.y + cam->config.heightFollow + dist * sinPitch,
        cam->targetPos.z + dist * cosPitch * cosf(cam->yaw),
    };
    cam->currentLookAt = (Vector3){
        cam->targetPos.x,
        cam->targetPos.y + cam->config.lookAtHeight,
        cam->targetPos.z,
    };

    cam->camera = (Camera3D){
        .position = cam->currentPos,
        .target = cam->currentLookAt,
        .up = (Vector3){ 0.0f, 1.0f, 0.0f },
        .fovy = 60.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Per-Frame Update
 *
 *  1. Advance mode transition timer
 *  2. Get current orbit parameters (distance, lateral, height)
 *  3. Compute desired camera position from spherical coordinates
 *  4. Smooth toward desired position
 *  5. TODO: Camera collision (raycast)
 *  6. Write Camera3D output
 * ═══════════════════════════════════════════════════════════════════════════ */

void GAME_CAMERA_Update(GameCamera* cam, float dt)
{
    /* ── 1. Mode transition ───────────────────────────────────────────── */
    if (cam->transitioning)
    {
        cam->transitionTimer += dt;
        if (cam->transitionTimer >= cam->config.transitionDuration)
        {
            cam->transitionTimer = cam->config.transitionDuration;
            cam->transitioning = false;
        }
    }

    /* ── 2. Current orbit parameters ──────────────────────────────────── */
    float dist, lateral, height;
    GetCurrentParams(cam, &dist, &lateral, &height);

    /* ── 3. Desired position from spherical coordinates ───────────────── */
    /*
     * The camera sits on a sphere around the target:
     *   - yaw rotates around Y axis (horizontal orbit)
     *   - pitch tilts up/down from the horizontal plane
     *   - dist is the radius of the sphere
     *
     * Then we add:
     *   - height: flat Y offset above target (independent of pitch)
     *   - lateral: offset perpendicular to orbit direction in XZ plane
     */
    float cosPitch = cosf(cam->pitch);
    float sinPitch = sinf(cam->pitch);

    /* Orbit direction in XZ (from target toward camera) */
    float orbitX = cosPitch * sinf(cam->yaw);
    float orbitZ = cosPitch * cosf(cam->yaw);

    /* Perpendicular to orbit in XZ (for lateral offset) */
    float rightX = cosf(cam->yaw);
    float rightZ = -sinf(cam->yaw);

    Vector3 desiredPos = {
        cam->targetPos.x + orbitX * dist + rightX * lateral,
        cam->targetPos.y + height + sinPitch * dist,
        cam->targetPos.z + orbitZ * dist + rightZ * lateral,
    };

    Vector3 desiredLookAt = {
        cam->targetPos.x,
        cam->targetPos.y + cam->config.lookAtHeight,
        cam->targetPos.z,
    };

    /* ── 4. Smooth interpolation (frame-rate independent) ─────────────── */
    float smoothFactor = 1.0f - expf(-cam->config.smoothSpeed * dt);

    cam->currentPos = Vector3Lerp(cam->currentPos, desiredPos, smoothFactor);
    cam->currentLookAt = Vector3Lerp(cam->currentLookAt, desiredLookAt, smoothFactor);

    /* ── 5. Camera collision (TODO: needs physics world from Fase 2) ──── */
    /*
     * Raycast from targetPos toward currentPos. If it hits scenery,
     * pull camera forward to hit point minus small margin.
     */

     /* ── 6. Update Camera3D output ────────────────────────────────────── */
    cam->camera.position = cam->currentPos;
    cam->camera.target = cam->currentLookAt;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Orbit Control
 *
 *  Mouse delta drives yaw (horizontal) and pitch (vertical).
 *  Pitch is clamped to prevent flipping over the poles.
 * ═══════════════════════════════════════════════════════════════════════════ */

void GAME_CAMERA_RotateByMouse(GameCamera* cam, float dx, float dy)
{
    cam->yaw -= dx * cam->config.mouseSensitivity * DEG2RAD;
    cam->pitch -= dy * cam->config.mouseSensitivity * DEG2RAD;

    /* Clamp pitch */
    float minRad = cam->config.pitchMin * DEG2RAD;
    float maxRad = cam->config.pitchMax * DEG2RAD;

    if (cam->pitch < minRad) cam->pitch = minRad;
    if (cam->pitch > maxRad) cam->pitch = maxRad;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Target Tracking
 * ═══════════════════════════════════════════════════════════════════════════ */

void GAME_CAMERA_SetTarget(GameCamera* cam, Vector3 pos)
{
    cam->targetPos = pos;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Direction Queries
 *
 *  These return the camera's facing directions projected onto the XZ plane
 *  (no Y component). Used by gameplay to move the character relative to
 *  where the camera is looking.
 *
 *  Forward = direction from camera toward target, projected onto XZ.
 *  Right   = perpendicular to forward in XZ, pointing right.
 * ═══════════════════════════════════════════════════════════════════════════ */

Vector3 GAME_CAMERA_GetForwardXZ(const GameCamera* cam)
{
    /*
     * The camera looks toward the target. The forward direction in XZ
     * is simply the opposite of the orbit direction (orbit points FROM
     * target TO camera, forward points FROM camera TO target).
     *
     * orbit direction = (sin(yaw), 0, cos(yaw))
     * forward = -orbit = (-sin(yaw), 0, -cos(yaw))
     */
    return (Vector3) { -sinf(cam->yaw), 0.0f, -cosf(cam->yaw) };
}

Vector3 GAME_CAMERA_GetRightXZ(const GameCamera* cam)
{
    /*
     * Right is perpendicular to forward in XZ, rotated 90° clockwise.
     * If forward = (-sin(yaw), 0, -cos(yaw)),
     * then right = (cos(yaw), 0, -sin(yaw))
     */
    return (Vector3) { cosf(cam->yaw), 0.0f, -sinf(cam->yaw) };
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Mode Control
 *
 *  Snapshots current params as start, sets destination from new mode.
 *  If called during a transition, restarts from the current eased values.
 * ═══════════════════════════════════════════════════════════════════════════ */

void GAME_CAMERA_SetMode(GameCamera* cam, CameraMode mode)
{
    if (mode == cam->mode && !cam->transitioning) return;

    /* Snapshot current values as starting point */
    GetCurrentParams(cam, &cam->distFrom, &cam->lateralFrom, &cam->heightFrom);
    GetParamsForMode(cam, mode, &cam->distTo, &cam->lateralTo, &cam->heightTo);

    cam->prevMode = cam->mode;
    cam->mode = mode;
    cam->transitioning = true;
    cam->transitionTimer = 0.0f;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Configuration
 * ═══════════════════════════════════════════════════════════════════════════ */

void GAME_CAMERA_SetConfig(GameCamera* cam, CameraConfig config)
{
    cam->config = config;
}