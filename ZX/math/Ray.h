#pragma once
#include <glm/glm.hpp>

struct Wall;

struct RaycastHit {
	float t;
	glm::vec3 normal;
	glm::vec3 point;
	Wall* wall;
};

struct Ray
{
	glm::vec3 origin;
	glm::vec3 direction;

	glm::vec3 At(float t) const
	{
		return origin + direction * t;
	}
};