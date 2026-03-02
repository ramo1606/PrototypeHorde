#pragma once

#include "SceneObject.h"
#include "raylib.h"
#include "raymath.h" 
#include "SceneCamera.h"

class FlyThroughCamera : public SceneCamera
{
public:

	FlyThroughCamera(Scene* Scene, const char* Name = nullptr, bool IsMainCamera = true);
	virtual ~FlyThroughCamera();

	void SetUp(Vector3 target, float dist, float yawAngle, float pitchAngle, float fovY, int projType);

	bool Update(float ElapsedSeconds) override;

	float distance = 20.0f;   // Distance from the target
	float yaw = 0;        // Horizontal angle (in radians)
	float pitch = -30;      // Vertical angle (in radians)

private:

	float sensitivity = 0.005f;
	float zoomSpeed = 0.5f;
	float moveSpeed = 0.2f;
};
