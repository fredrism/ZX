#include <iostream>
#include "raylib.h"
#include "glm/glm.hpp"
#include "glm/common.hpp"
#include "glm/gtc/quaternion.hpp"

#include <vector>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

constexpr float aspectRatio = (float)SCREEN_WIDTH / SCREEN_HEIGHT;

struct Plane
{
	glm::vec3 direction;
	glm::vec3 center;
};

struct ZXMaterial
{
	Color color;
	float roughness;
};

struct Wall
{
	unsigned int materialId = 0;
	int portal = -1;
};

struct Sector 
{
	std::vector<glm::vec3> boundary;
	std::vector<Wall> walls;
	std::vector<ZXMaterial> materials;

	Wall topInfo;
	Wall floorInfo;
	Plane bottom;
	Plane top;

	glm::vec3 GetPoint(unsigned int index)
	{
		return boundary[index % boundary.size()];
	}

	void GetBoundaryPlanes(unsigned int index, Plane& wall, Plane& left, Plane& right)
	{
		glm::vec3 p0 = GetPoint(index);
		glm::vec3 p1 = GetPoint(index + 1.0f);

		glm::vec3 dir = glm::normalize(p1 - p0);
		glm::vec3 normal = glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f));

		wall.center = p0;
		wall.direction = normal;

		left.center = p0;
		left.direction = dir;

		right.center = p1;
		right.direction = -dir;
	}
};

struct ZXTransform
{
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
	glm::mat4 transform;

	ZXTransform()
	{
		position = glm::vec3(0, 0, 0);
		rotation = glm::vec3(0, 0, 0);
		scale = glm::vec3(1, 1, 1);
		transform = glm::mat4(1.0);
	}

	glm::mat4 localToWorld()
	{
		glm::mat4 matrix(1.0);
		matrix = glm::translate(matrix, position);
		matrix = glm::scale(matrix, scale);
		matrix = glm::rotate(matrix, rotation.x, glm::vec3(1, 0, 0));
		matrix = glm::rotate(matrix, rotation.y, glm::vec3(0, 1 ,0));
		matrix = glm::rotate(matrix, rotation.z, glm::vec3(0, 0, 1));

		this->transform = matrix;
		return matrix;
	}

	glm::vec3 Forward()
	{
		return glm::mat4_cast(toQuaternion()) * glm::vec4(0, 0, -1, 0);
	}

private:
	glm::quat toQuaternion() const
	{
		return glm::angleAxis(rotation.x, glm::vec3(1, 0, 0)) * glm::angleAxis(rotation.y, glm::vec3(0, 1, 0)) * glm::angleAxis(rotation.z, glm::vec3(0, 0, 1));
	}
};

struct ZXRay
{
	glm::vec3 origin;
	glm::vec3 direction;

	glm::vec3 At(float t)
	{
		return origin + direction * t;
	}
};

struct ZXCamera
{
	ZXTransform transform;
	float fieldOfView = 60;

	ZXRay ScreenPointToRay(float x, float y)
	{
		float fov = glm::tan(glm::radians(fieldOfView * 0.5f));
		float px = (2.0f * (x + 0.5f) / SCREEN_WIDTH - 1.0f) * fov * aspectRatio;
		float py = (1.0f - 2.0f * (y + 0.5f) / SCREEN_HEIGHT) * fov;

		glm::vec4 rayOrigin = transform.transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		glm::vec4 rayPos = transform.transform * glm::vec4(px, py, -1.0f, 1.0f);

		ZXRay ray;
		ray.origin = rayOrigin;
		ray.direction = rayPos - rayOrigin;
		ray.direction = glm::normalize(ray.direction);
		return ray;
	}
};

float Cross2D(glm::vec2 a, glm::vec2 b)
{
	return a.x * b.y - a.y * b.x;
}

float LineLineIntersection(glm::vec2 p1, glm::vec2 p2, glm::vec2 q1, glm::vec2 q2)
{
	glm::vec2 r = p2 - p1;
	glm::vec2 s = q2 - q1;

	return Cross2D(q1 - p1, s / Cross2D(r, s));
}

bool RayPlaneIntersection(ZXRay ray, Plane p, float& t)
{
	float d = glm::dot(p.direction, ray.direction);
	if (glm::abs(d) > 0.0001f)
	{
		t = glm::dot(p.center - ray.origin, p.direction) / d;
		if (t >= 0.0f)
		{
			return true;
		}
	}

	return false;
}

float SideOf(glm::vec3 point, Plane p)
{
	return glm::dot(point - p.center, p.direction);
}

struct HitInfo
{
	float t;
	glm::vec3 point;
	Wall info;
};

HitInfo RayCast(ZXRay ray, Sector* sector)
{
	Plane planes[3];
	for (int i = 0; i < sector->boundary.size(); i++)
	{
		sector->GetBoundaryPlanes(i, planes[0], planes[1], planes[2]);
		float t = -1;
		if (RayPlaneIntersection(ray, planes[0], t))
		{
			glm::vec3 hitPoint = ray.At(t);

			if (SideOf(hitPoint, planes[1]) > 0 &&
				SideOf(hitPoint, planes[2]) > 0 &&
				SideOf(hitPoint, sector->top) > 0 &&
				SideOf(hitPoint, sector->bottom) > 0)
			{
				return {
					t,
					hitPoint,
					sector->walls[i]
				};
			}
		}
	}
}

void Render(Sector* sector, ZXCamera* camera)
{
	camera->transform.localToWorld();

	Color roof{ 255, 0, 0, 255 };
	Color sky{ 0, 255, 0, 255 };
	ZXRay ray;

	Plane planes[3];
	Color colors[2] = {
		{128, 90, 144, 255},
		{23, 44, 200, 255}
	};

	for (int y = 0; y < SCREEN_HEIGHT; y++)
	{
		for (int x = 0; x < SCREEN_WIDTH; x++)
		{
			ZXRay ray = camera->ScreenPointToRay(x, y);

			for (int i = 0; i < sector->boundary.size(); i++)
			{
				sector->GetBoundaryPlanes(i, planes[0], planes[1], planes[2]);
				float t = -1;
				if (RayPlaneIntersection(ray, planes[0], t))
				{
					glm::vec3 hitPoint = ray.At(t);

					if (SideOf(hitPoint, planes[1]) > 0 &&
						SideOf(hitPoint, planes[2]) > 0 &&
						SideOf(hitPoint, sector->top) > 0 &&
						SideOf(hitPoint, sector->bottom) > 0)
					{
						DrawPixel(x, y, colors[i % 2]);
					}
				}
			}
		}
	}
}

void DrawMinimap(ZXCamera* camera, Sector* sector)
{
	const float scale = 2;
	glm::vec2 offset(50, 50);
	glm::vec3 lookDir = camera->transform.Forward();

	for (int i = 1; i <= sector->boundary.size(); i++)
	{
		glm::vec3 p0 = scale * (sector->boundary[i - 1] - camera->transform.position);
		glm::vec3 p1 = scale * (sector->boundary[(i % sector->boundary.size())] - camera->transform.position);

		glm::vec2 l0(p0.x, p0.z);
		glm::vec2 l1(p1.x, p1.z);

		DrawLine(l0.x + offset.x, l0.y + offset.y, l1.x + offset.x, l1.y + offset.y, { 0, 0, 255, 255 });
	}

	DrawLine(offset.x, offset.y, offset.x + 20 * lookDir.x, offset.y + 20 * lookDir.z, { 255, 0, 0, 255 });
}

void main()
{
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "ZX");
	SetTargetFPS(60);

	ZXCamera camera;
	//camera.transform.position = glm::vec3(0, 0, 0);
	Sector sector;
	sector.bottom.center = glm::vec3(0, -1, 0);
	sector.bottom.direction = glm::vec3(0, 1, 0);
	sector.top.center = glm::vec3(0, 1, 0);
	sector.top.direction = glm::vec3(0, -1, 0);
	sector.materials.push_back({
		{71, 63, 78, 255},
		1.0f
	});

	sector.walls.push_back({ 0, -1 });
	sector.walls.push_back({ 0, -1 });
	sector.walls.push_back({ 0, -1 });
	sector.walls.push_back({ 0, -1 });

	sector.boundary.push_back(glm::vec3(-20, 0, -20));
	sector.boundary.push_back(glm::vec3(20, 0, -20));
	sector.boundary.push_back(glm::vec3(20, 0, 20));
	sector.boundary.push_back(glm::vec3(-20, 0, 20));

	while (!WindowShouldClose())
	{
		float dt = 1 / 60.0;

		if (IsKeyDown(KEY_A))
		{
			camera.transform.rotation.y += 1.0 * dt;
		}
		if (IsKeyDown(KEY_D))
		{
			camera.transform.rotation.y -= 1.0 * dt;
		}
		if (IsKeyDown(KEY_W))
		{
			camera.transform.position += camera.transform.Forward() * 10.0f * dt;
		}
		if (IsKeyDown(KEY_S))
		{
			camera.transform.position -= camera.transform.Forward() * 10.0f * dt;
		}

		//std::cout << camera.transform.position.x << ", " << camera.transform.position.y << ", " << camera.transform.position.z << std::endl;

		BeginDrawing();
		ClearBackground(RAYWHITE);
		Render(&sector, &camera);
		DrawMinimap(&camera, &sector);
		EndDrawing();
	}

	CloseWindow();
}