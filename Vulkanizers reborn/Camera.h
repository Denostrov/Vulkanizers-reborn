#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
	Camera();
	Camera(glm::vec3 const& position, float focalLength);

	float focalLength;
	glm::vec3 position;
};