#pragma once

#include "rlgl.h"

#include "OrthogonalCamera.h"

typedef enum {
	DIRECTIONAL_LIGHT = 0, // Directional light (infinite distance)
	POINT_LIGHT,           // Point light (omnidirectional)
} SceneLightType;

class ShadowSceneLight : public OrthogonalCamera
{
	public:
		ShadowSceneLight(Scene* Scene, const char* Name = nullptr);

		bool Update(float ElapsedSeconds) override;

		virtual void SetLight(Vector3 dir, Color col);

		// Record the light matrices for future use!
		Matrix lightView = { 0 };
		Matrix lightProj = { 0 };
		Matrix lightViewProj = { 0 };
		Vector3 lightDir = { 0 };
		Color lightColor = WHITE;
		Color lightAmbient = CLITERAL(Color) { 26, 26, 26, 255 };
		SceneLightType lightType = DIRECTIONAL_LIGHT;
};



