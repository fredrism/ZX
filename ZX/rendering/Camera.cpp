#include "Camera.h"

Camera::Camera()
{
    this->SetFOV(60);
}

Camera::~Camera()
{
}

void Camera::SetFOV(float degrees)
{
	this->m_fieldOfView = degrees;
	this->m_rayOffset = glm::tan(glm::radians(degrees * 0.5f));
}

float Camera::GetFOV() const
{
	return this->m_fieldOfView;
}

Ray Camera::ScreenPointToRay(float x, float y) const
{
	
	float px = (2.0f * (x + 0.5f) / SCREEN_WIDTH - 1.0f) * this->m_rayOffset * aspectRatio;
	float py = (1.0f - 2.0f * (y + 0.5f) / SCREEN_HEIGHT) * this->m_rayOffset;

	glm::vec4 rayOrigin = this->m_transform.transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec4 rayPos = this->m_transform.transform * glm::vec4(px, py, -1.0f, 1.0f);

	Ray ray;
	ray.origin = rayOrigin;
	ray.direction = rayPos - rayOrigin;
	ray.direction = glm::normalize(ray.direction);
	return ray;
}

Transform& Camera::GetTransform()
{
	return this->m_transform;
}
