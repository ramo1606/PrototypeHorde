#include "Knight.h"
#include "WaypointsCamera.h"

WaypointsCamera::WaypointsCamera(Scene* Scene, const char* Name, bool IsMainCamera)
    : SceneCamera(Scene, Name, IsMainCamera)
{
    _Camera.projection = CAMERA_PERSPECTIVE;
}

bool WaypointsCamera::Update(float ElapsedSeconds)
{
    if (!IsActive)
        return false;

    // Check if we need to wait at the current waypoint
    if (waypointWaitTimer > 0.0f) {
        waypointWaitTimer -= GetFrameTime();
    } else {
        // Calculate direction and distance to the next waypoint
        Vector3 direction = Vector3Subtract(waypoints[currentWaypoint].position, _Camera.position);
        float distance = Vector3Length(direction);

        // If we're close enough to the waypoint, move to the next one
        if (distance < 0.1f) {
            currentWaypoint = (currentWaypoint + 1) % waypoints.size();
            waypointWaitTimer = waypoints[currentWaypoint].waitTime;
        }
        else {
            // Normalize direction and move the camera
            direction = Vector3Scale(Vector3Normalize(direction), moveSpeed * GetFrameTime());
            _Camera.position = Vector3Add(_Camera.position, direction);
        }

        // Update camera target to follow the moving cube
        _Camera.target = Vector3Lerp(_Camera.target, TargetActor->Position, 0.1f);
    }

    return true;
}

void WaypointsCamera::SetUp(SceneActor* pTarget, float fovY, int projType)
{
    // Set up the camera with a third-person perspective	
    _Camera.position = Vector3{ 0.0f, 2.0f, -5.0f }; // Initial position of the camera
    _Camera.fovy = fovY; // Field of view
    _Camera.up = Vector3{ 0.0f, 1.0f, 0.0f }; // Up direction of the camera
    TargetActor = pTarget;
    _Camera.target = TargetActor->Position; // Focus on the player
    _Camera.projection = projType;
}

bool WaypointsCamera::Draw()
{
    // Draw a cube at each waypoint for visualization
    for (const auto& waypoint : waypoints) {
        DrawCube(waypoint.position, 0.5f, 0.5f, 0.5f, RED);
        DrawCubeWires(waypoint.position, 0.5f, 0.5f, 0.5f, DARKGRAY);
    }

    return true;
}

