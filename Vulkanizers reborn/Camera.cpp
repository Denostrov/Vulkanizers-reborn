#include "Camera.h"

Camera::Camera()
	:position{ glm::vec3(0.0f, 0.0f, 0.0f) }, focalLength{ 1.0 }, pitch{ 0.0f }, yaw{ 0.0f }, speed{ 0.5f }
{
	updateDirection();
	calculateUpAndRight();
}

Camera::Camera(glm::vec3 const& position, glm::vec3 const& target, float focalLength)
	: position{ position }, focalLength{ focalLength }, direction{ glm::normalize(target - position) }, speed{ 0.5f }
{
	calculateEulerAngles();
	calculateUpAndRight();
}

void Camera::orient(float pitch, float yaw)
{
	this->pitch = pitch;
	this->yaw = yaw;
	updateDirection();
	calculateUpAndRight();
}

void Camera::point(glm::vec3 const& target)
{
	direction = glm::normalize(target - position);
	calculateEulerAngles();
}

void Camera::updateDirection()
{
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
}

void Camera::calculateUpAndRight()
{
	right = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
	up = glm::normalize(glm::cross(right, direction));
}

void Camera::calculateEulerAngles()
{
	pitch = glm::degrees(asin(direction.y));
	yaw = glm::degrees(atan2(direction.z, direction.x));
}