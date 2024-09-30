#include "Sector.h"
bool Sector::Raycast(const Ray& ray, RaycastHit& hit) const
{
	for (int i = 0; i < boundary.size(); i++)
	{
		const Plane& left = wallPlanes[3 * i];
		const Plane& wall = wallPlanes[3 * i + 1];
		const Plane& right = wallPlanes[3 * i + 2];
		if (wall.Raycast(ray, hit.t))
		{
			hit.point = ray.At(hit.t);

			if (left.SideOf(hit.point) > 0 &&
				right.SideOf(hit.point) > 0 &&
				top.SideOf(hit.point) > 0 &&
				bottom.SideOf(hit.point) > 0)
			{
				hit.normal = wall.normal;
				return true;
			}
		}
	}


	// Need to make sure we pick the right intersection
	float topDistance = 0, bottomDistance = 0;
	bool hitFloorOrCeil = false;
	if (top.Raycast(ray, topDistance))
	{
		hit.point = ray.At(topDistance);
		hit.normal = top.normal;
		hitFloorOrCeil = true;
	}
	if (bottom.Raycast(ray, bottomDistance))
	{
		if (!hitFloorOrCeil || glm::distance(ray.origin, ray.At(bottomDistance)) < glm::distance(ray.origin, ray.At(topDistance)))
		{
			hit.point = ray.At(bottomDistance);
			hit.normal = bottom.normal;
			hitFloorOrCeil = true;
		}
	}
	return hitFloorOrCeil;
}