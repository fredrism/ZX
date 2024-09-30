#pragma once
#include "../math/Plane.h"
#include "../rendering/Material.h"
#include "../math/Ray.h"
#include "glm/glm.hpp"
#include <vector>

struct Wall {
	unsigned int materialId = 0;
	int portal = -1;
};


class Sector {
public:
	std::vector<glm::vec3> boundary;
	std::vector<Wall> walls;
	std::vector<Plane> wallPlanes;
	std::vector<Material> materials;

	Wall topInfo;
	Wall floorInfo;
	Plane bottom;
	Plane top;

	bool Raycast(const Ray& ray, RaycastHit& hit) const;

	inline glm::vec3 GetPoint(unsigned int index) const
	{
		return boundary[index % boundary.size()];
	}

	void ComputePlanes() {
		for (int i = 0; i < boundary.size(); i++) {
			glm::vec3 p0 = GetPoint(i);
			glm::vec3 p1 = GetPoint(i + 1);

			glm::vec3 dir = glm::normalize(p1 - p0);
			glm::vec3 normal = glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f));

			Plane wall, left, right;

			wall.center = p0;
			wall.normal = normal;

			left.center = p0;
			left.normal = dir;

			right.center = p1;
			right.normal = -dir;

			wallPlanes.push_back(left);
			wallPlanes.push_back(wall);
			wallPlanes.push_back(right);
		}
	}
};