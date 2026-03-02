#pragma once

#include <vector>

#include "SceneObject.h"
#include "raylib.h"
#include "raymath.h" 
#include "SceneCamera.h"

struct Waypoint {
	Vector3 position;
	float waitTime; // Optional wait time at the waypoint
};

class WaypointsCamera : public SceneCamera
{
public:

	WaypointsCamera(Scene* Scene, const char* Name = nullptr, bool IsMainCamera = true);

	void SetUp(SceneActor* pTarget, float fovY, int projType);

	bool Update(float ElapsedSeconds) override;
	bool Draw() override;

	//target Actor to follow up
	SceneActor* TargetActor = NULL;

	int currentWaypoint = 0;         // Current waypoint index
	float moveSpeed = 1.0f;          // Speed of movement between waypoints
	float waypointWaitTimer = -1.0f;  // Timer to handle waiting at each waypoint

	// Define a set of waypoints for the camera to move along
	std::vector<Waypoint> waypoints = {
		{{ 0.0f, 4.0f, -10.0f }, 3.0f},
		{{ 5.0f, 3.0f, -5.0f }, 3.0f},
		{{ 10.0f, 5.0f, 0.0f }, 3.0f},
		{{ 5.0f, 4.0f, 5.0f }, 3.0f},
		{{ 0.0f, 5.0f, 10.0f }, 3.0f},
		{{ -5.0f, 3.0f, 5.0f }, 3.0f}
	};
};

