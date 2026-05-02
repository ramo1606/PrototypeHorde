#include "camera.h"
#include "raymath.h"
#include "reasings.h"

#include <math.h>

static const CameraConfig DEFAULT_CONFIG = {
    .distFollow         = 10.0f,
    .distAim            = 5.0f,
    .lateralFollow      = 0.0f,
    .lateralAim         = 1.5f,
    .heightFollow       = 4.0f,
    .heightAim          = 2.5f,
    .mouseSensitivity   = 0.15f,
    .pitchMin           = -30.0f,
    .pitchMax           = 60.0f,
    .smoothSpeed        = 10.0f,
    .transitionDuration = 0.3f,
    .lookAtHeight       = 1.2f,
};

/* ── Helpers ─────────────────────────────────────────────────────────────── */

static void getCurrentParams(const GameCamera* cam,
                             float* outDist, float* outLateral, float* outHeight)
{
    if (cam->transitioning)
    {
        float t = cam->transitionTimer;
        float d = cam->config.transitionDuration;

        *outDist    = EaseSineInOut(t, cam->distFrom,    cam->distTo    - cam->distFrom,    d);
        *outLateral = EaseSineInOut(t, cam->lateralFrom, cam->lateralTo - cam->lateralFrom, d);
        *outHeight  = EaseSineInOut(t, cam->heightFrom,  cam->heightTo  - cam->heightFrom,  d);
    }
    else
    {
        switch (cam->mode)
        {
        case CAMERA_MODE_AIM:
            *outDist    = cam->config.distAim;
            *outLateral = cam->config.lateralAim;
            *outHeight  = cam->config.heightAim;
            break;
        case CAMERA_MODE_FOLLOW:
        default:
            *outDist    = cam->config.distFollow;
            *outLateral = cam->config.lateralFollow;
            *outHeight  = cam->config.heightFollow;
            break;
        }
    }
}

static void getParamsForMode(const GameCamera* cam, CameraMode mode,
                             float* outDist, float* outLateral, float* outHeight)
{
    switch (mode)
    {
    case CAMERA_MODE_AIM:
        *outDist    = cam->config.distAim;
        *outLateral = cam->config.lateralAim;
        *outHeight  = cam->config.heightAim;
        break;
    case CAMERA_MODE_FOLLOW:
    default:
        *outDist    = cam->config.distFollow;
        *outLateral = cam->config.lateralFollow;
        *outHeight  = cam->config.heightFollow;
        break;
    }
}

/* ── Lifecycle ───────────────────────────────────────────────────────────── */

void CameraInit(GameCamera* cam)
{
    cam->config   = DEFAULT_CONFIG;
    cam->mode     = CAMERA_MODE_FOLLOW;
    cam->prevMode = CAMERA_MODE_FOLLOW;

    cam->yaw   = 0.0f;
    cam->pitch = 20.0f * DEG2RAD;

    cam->targetPos = (Vector3){ 0.0f, 0.0f, 0.0f };

    cam->transitioning   = false;
    cam->transitionTimer = 0.0f;

    float cosPitch = cosf(cam->pitch);
    float sinPitch = sinf(cam->pitch);
    float dist     = cam->config.distFollow;

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
        .position   = cam->currentPos,
        .target     = cam->currentLookAt,
        .up         = (Vector3){ 0.0f, 1.0f, 0.0f },
        .fovy       = 60.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
}

/* ── Per-Frame Update ────────────────────────────────────────────────────── */

void CameraUpdate(GameCamera* cam, float dt)
{
    if (cam->transitioning)
    {
        cam->transitionTimer += dt;
        if (cam->transitionTimer >= cam->config.transitionDuration)
        {
            cam->transitionTimer = cam->config.transitionDuration;
            cam->transitioning   = false;
        }
    }

    float dist, lateral, height;
    getCurrentParams(cam, &dist, &lateral, &height);

    float cosPitch = cosf(cam->pitch);
    float sinPitch = sinf(cam->pitch);

    float orbitX = cosPitch * sinf(cam->yaw);
    float orbitZ = cosPitch * cosf(cam->yaw);

    float rightX =  cosf(cam->yaw);
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

    /* Frame-rate independent exponential smoothing */
    float smoothFactor = 1.0f - expf(-cam->config.smoothSpeed * dt);

    cam->currentPos    = Vector3Lerp(cam->currentPos,    desiredPos,    smoothFactor);
    cam->currentLookAt = Vector3Lerp(cam->currentLookAt, desiredLookAt, smoothFactor);

    cam->camera.position = cam->currentPos;
    cam->camera.target   = cam->currentLookAt;
}

/* ── Orbit Control ───────────────────────────────────────────────────────── */

void CameraRotateByMouse(GameCamera* cam, float dx, float dy)
{
    cam->yaw   -= dx * cam->config.mouseSensitivity * DEG2RAD;
    cam->pitch -= dy * cam->config.mouseSensitivity * DEG2RAD;

    float minRad = cam->config.pitchMin * DEG2RAD;
    float maxRad = cam->config.pitchMax * DEG2RAD;

    if (cam->pitch < minRad) cam->pitch = minRad;
    if (cam->pitch > maxRad) cam->pitch = maxRad;
}

/* ── Target Tracking ─────────────────────────────────────────────────────── */

void CameraSetTarget(GameCamera* cam, Vector3 pos)
{
    cam->targetPos = pos;
}

/* ── Direction Queries ───────────────────────────────────────────────────── */

Vector3 CameraGetForwardXZ(const GameCamera* cam)
{
    return (Vector3) { -sinf(cam->yaw), 0.0f, -cosf(cam->yaw) };
}

Vector3 CameraGetRightXZ(const GameCamera* cam)
{
    return (Vector3) { cosf(cam->yaw), 0.0f, -sinf(cam->yaw) };
}

/* ── Mode Control ────────────────────────────────────────────────────────── */

void CameraSetMode(GameCamera* cam, CameraMode mode)
{
    if (mode == cam->mode && !cam->transitioning) return;

    getCurrentParams(cam, &cam->distFrom, &cam->lateralFrom, &cam->heightFrom);
    getParamsForMode(cam, mode, &cam->distTo, &cam->lateralTo, &cam->heightTo);

    cam->prevMode        = cam->mode;
    cam->mode            = mode;
    cam->transitioning   = true;
    cam->transitionTimer = 0.0f;
}

/* ── Configuration ───────────────────────────────────────────────────────── */

void CameraSetConfig(GameCamera* cam, CameraConfig config)
{
    cam->config = config;
}
