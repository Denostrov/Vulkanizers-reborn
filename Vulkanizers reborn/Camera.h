#pragma once

#include <glm/glm.hpp>
#include "Constants.h"

class Camera
{
public:
	Camera();
	Camera(glm::vec3 const& position, glm::vec3 const& target, float focalLength);

	void orient(float pitch, float yaw);
	void point(glm::vec3 const& target);

	float focalLength;
	float pitch;
	float yaw;
	float speed;
	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 right;
	glm::vec3 up;

private:
	void updateDirection();
	void calculateUpAndRight();
	void calculateEulerAngles();
};