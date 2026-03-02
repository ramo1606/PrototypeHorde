#pragma once

#include "SceneActor.h"

class SceneLight : public SceneActor
{
public:
	SceneLight(Scene* Scene, const char* Name = nullptr);
	virtual ~SceneLight();

	void Create();
	bool Update(float ElapsedSeconds) override;

	Shader shader = { 0 };
	
	// Set up directional light properties
	Vector3 lightDirection = { -1,-1,-1 }; // Direction towards the light source
	Vector3 lightColor = { 1.0f, 1.0f, 1.0f };       // White light
	Vector3 ambientColor = { 0.2f, 0.2f, 0.2f };

protected:
	friend class Knight;

	int locLightDirection = -1;
	int locLightPosition = -1;
	int locLightColor = -1;
	int locAmbientColor = -1;
};

//End of SceneLight.h
