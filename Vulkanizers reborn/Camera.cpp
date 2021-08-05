#include "Camera.h"

Camera::Camera()
	:position{ glm::vec3(0.0f, 0.0f, 0.0f) }, focalLength{ 1.0 }
{

}

Camera::Camera(glm::vec3 const& position, float focalLength)
	: position{ position }, focalLength{ focalLength }
{

}