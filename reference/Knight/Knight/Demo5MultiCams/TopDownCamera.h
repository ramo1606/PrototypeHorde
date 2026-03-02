#pragma once

#include "SceneObject.h"
#include "raylib.h"
#include "raymath.h" 
#include "SceneCamera.h"

class TopDownCamera : public SceneCamera
{
public:

	TopDownCamera(Scene* Scene, const char* Name = nullptr, bool IsMainCamera = true);
	virtual ~TopDownCamera();

	void SetUp(Vector3 pos, Vector3 target, float fovY, int projType);

	bool Update(float ElapsedSeconds) override;

	float cameraZoom = 15.0f;  // Initial camera distance

	float cameraPanSpeed = 0.1f;  // Camera panning speed
	float zoomSpeed = 1.0f;  // Zoom speed

private:
	float mCameraHorizontalAngleShift = 0.0f;
	float mCameraVerticalOffset = 0.0f;

};
