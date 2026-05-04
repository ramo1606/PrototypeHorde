#pragma once

#include "camera_types.h"

/* Initialize with sensible isometric defaults. */
void CameraInit(GameCamera* cam);

/* Per-frame update: smooth toward target, recompute Camera3D. */
void CameraUpdate(GameCamera* cam, float dt);

/* Set the target position the camera follows. Call once per tick (or
 * whenever the target moves). The smoothing happens in CameraUpdate. */
void CameraSetTarget(GameCamera* cam, Vector3 pos);

/* Multiplayer: set centroid and spread together. The distance interpolates
 * between multiplayerMinDistance and multiplayerMaxDistance based on
 * spread * multiplayerSpreadFactor. */
void CameraSetGroupTarget(GameCamera* cam, Vector3 centroid, float spread);

/* Replace the whole config. */
void CameraSetConfig(GameCamera* cam, CameraConfig config);
