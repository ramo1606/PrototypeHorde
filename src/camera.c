#include "camera.h"
#include "raymath.h"

#include <math.h>

static const CameraConfig DEFAULT_CONFIG = {
    .elevationDeg            = 30.0f,
    .azimuthDeg              = 45.0f,
    .fovy                    = 30.0f,
    .distance                = 14.0f,
    .lookAtHeight            = 0.8f,
    .smoothSpeed             = 8.0f,
    .multiplayerMinDistance  = 14.0f,
    .multiplayerMaxDistance  = 26.0f,
    .multiplayerSpreadFactor = 1.5f,
};

/* Cartesian offset from look-at to camera, given iso angles + distance.
 * Spherical: x = d * cos(elev) * sin(azim)
 *            y = d * sin(elev)
 *            z = d * cos(elev) * cos(azim)
 */
static Vector3 cameraOffsetFromConfig(const CameraConfig* c, float distance)
{
    float elev = c->elevationDeg * DEG2RAD;
    float azim = c->azimuthDeg   * DEG2RAD;
    float ce = cosf(elev), se = sinf(elev);
    float ca = cosf(azim), sa = sinf(azim);

    return (Vector3){
        distance * ce * sa,
        distance * se,
        distance * ce * ca,
    };
}

/* Distance to use this frame. Scales with spread when multiplayer is
 * active (spread > 0). Clamped to [min, max]. */
static float currentDistance(const GameCamera* cam)
{
    if (cam->spread <= 0.0f) return cam->config.distance;

    float scaled = cam->config.multiplayerMinDistance
                 + cam->spread * cam->config.multiplayerSpreadFactor;
    if (scaled < cam->config.multiplayerMinDistance) scaled = cam->config.multiplayerMinDistance;
    if (scaled > cam->config.multiplayerMaxDistance) scaled = cam->config.multiplayerMaxDistance;
    return scaled;
}

void CameraInit(GameCamera* cam)
{
    cam->config        = DEFAULT_CONFIG;
    cam->targetPos     = (Vector3){ 0.0f, 0.0f, 0.0f };
    cam->spread        = 0.0f;
    cam->currentLookAt = (Vector3){ 0.0f, cam->config.lookAtHeight, 0.0f };

    Vector3 offset = cameraOffsetFromConfig(&cam->config, cam->config.distance);
    cam->camera = (Camera3D){
        .position   = Vector3Add(cam->currentLookAt, offset),
        .target     = cam->currentLookAt,
        .up         = (Vector3){ 0.0f, 1.0f, 0.0f },
        .fovy       = cam->config.fovy,
        .projection = CAMERA_PERSPECTIVE,
    };
}

void CameraUpdate(GameCamera* cam, float dt)
{
    Vector3 desiredLookAt = {
        cam->targetPos.x,
        cam->targetPos.y + cam->config.lookAtHeight,
        cam->targetPos.z,
    };

    /* Frame-rate independent exponential smoothing. */
    float t = 1.0f - expf(-cam->config.smoothSpeed * dt);
    cam->currentLookAt = Vector3Lerp(cam->currentLookAt, desiredLookAt, t);

    Vector3 offset = cameraOffsetFromConfig(&cam->config, currentDistance(cam));
    cam->camera.position = Vector3Add(cam->currentLookAt, offset);
    cam->camera.target   = cam->currentLookAt;
    cam->camera.fovy     = cam->config.fovy;
}

void CameraSetTarget(GameCamera* cam, Vector3 pos)
{
    cam->targetPos = pos;
    cam->spread    = 0.0f;
}

void CameraSetGroupTarget(GameCamera* cam, Vector3 centroid, float spread)
{
    cam->targetPos = centroid;
    cam->spread    = spread;
}

void CameraSetConfig(GameCamera* cam, CameraConfig config)
{
    cam->config = config;
}
