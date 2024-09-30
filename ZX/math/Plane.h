#pragma once
#include "glm/glm.hpp"
#include "Ray.h"

class Plane
{
public:
	Plane(glm::vec3 center, glm::vec3 normal)
	{
		this->center = center;
		this->normal = normal;
	}

	Plane(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
	{
		glm::vec3 normal = glm::cross(p2 - p1, p3 - p1);
		this->center = p1;
		this->normal = normal;
	}

	bool RayPlaneIntersection(Ray ray, float& t)
	{
		float d = glm::dot(normal, ray.direction);
		if (glm::abs(d) > 0.0001f)
		{
			t = glm::dot(center - ray.origin, normal) / d;
			if (t >= 0.0f)
			{
				return true;
			}
		}

		return false;
	}

	float SideOf(glm::vec3 point)
	{
		return glm::dot(point - center, normal);
	}

public:
	glm::vec3 center;
	glm::vec3 normal;
};