#include "PerspectiveCamera.h"
#include "Scene.h"

PerspectiveCamera::PerspectiveCamera(Scene* Scene, const char* Name, bool IsMainCamera)
	: SceneCamera(Scene, Name, IsMainCamera)
{
	_Camera.projection = CAMERA_PERSPECTIVE;
}

PerspectiveCamera::~PerspectiveCamera()
{

}

