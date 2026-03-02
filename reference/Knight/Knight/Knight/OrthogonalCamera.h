#pragma once

#include "SceneObject.h"
#include "raylib.h"
#include "raymath.h" 

#include "SceneCamera.h"

class OrthogonalCamera : public SceneCamera
{
public:

	OrthogonalCamera(Scene* Scene, const char* Name = nullptr, bool IsMainCamera = true);
	virtual ~OrthogonalCamera();

	void SetUp(Vector3 pos, Vector3 target, float zoom);

	bool Update(float ElapsedSeconds) override;

	// Variables to control camera zoom and pan
	float cameraZoom = 10.0f;
	float panSpeed = 0.1f;

protected:
	friend class Knight;

	float cameraHorizontalAngleShift = 0.0f;
	float cameraVerticalOffset = 0.0f;
};
