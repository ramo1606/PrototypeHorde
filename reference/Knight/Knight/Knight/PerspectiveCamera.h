#pragma once

#include "SceneObject.h"
#include "raylib.h"
#include "raymath.h" 
#include "SceneCamera.h"

class PerspectiveCamera : public SceneCamera
{
public:
	
	PerspectiveCamera(Scene* Scene, const char* Name = nullptr, bool IsMainCamera = true);
	virtual ~PerspectiveCamera();

private:
	friend class Knight;
};