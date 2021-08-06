#include "Camera.h"

Camera::Camera()
	:position{ glm::vec3(0.0f, 0.0f, 0.0f) }, focalLength{ 1.0 }, direction{ glm::vec3(0.0f, 0.0f, 1.0f) }
{
	right = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
	up = glm::normalize(glm::cross(direction, right));
}

Camera::Camera(glm::vec3 const& position, glm::vec3 const& target, float focalLength)
	: position{ position }, focalLength{ focalLength }, direction{ glm::normalize(target - position) }
{
	right = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
	up = glm::normalize(glm::cross(direction, right));
}