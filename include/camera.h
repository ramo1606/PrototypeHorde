#pragma once

#include "raylib.h"

typedef struct CameraConfig
{
    float elevationDeg;
    float azimuthDeg;
    float fovy;
    float distance;
    float lookAtHeight;
    float smoothSpeed;
    float multiplayerMinDistance;
    float multiplayerMaxDistance;
    float multiplayerSpreadFactor;
} CameraConfig;

typedef struct GameCamera
{
    Camera3D camera;
    CameraConfig config;
    Vector3 targetPos;
    float spread;
    Vector3 currentLookAt;
} GameCamera;

void CameraInit(GameCamera* cam);
void CameraUpdate(GameCamera* cam, float dt);
void CameraSetTarget(GameCamera* cam, Vector3 pos);
void CameraSetGroupTarget(GameCamera* cam, Vector3 centroid, float spread);
void CameraSetConfig(GameCamera* cam, CameraConfig config);
