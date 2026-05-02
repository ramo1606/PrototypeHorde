#pragma once

#include "camera_types.h"

/*
 * NOTE: this is the legacy third-person orbit camera. It will be replaced
 * in Task 1.6 with a fixed isometric camera (see CLAUDE.md). For now we
 * keep the API working so the rest of the project compiles.
 */

void CameraInit(GameCamera* cam);
void CameraUpdate(GameCamera* cam, float dt);

void CameraRotateByMouse(GameCamera* cam, float dx, float dy);

void CameraSetTarget(GameCamera* cam, Vector3 pos);

Vector3 CameraGetForwardXZ(const GameCamera* cam);
Vector3 CameraGetRightXZ(const GameCamera* cam);

void CameraSetMode(GameCamera* cam, CameraMode mode);
void CameraSetConfig(GameCamera* cam, CameraConfig config);
