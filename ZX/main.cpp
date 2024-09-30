#include <iostream>
#include "raylib.h"
#include "glm/glm.hpp"
#include "glm/common.hpp"
#include "glm/gtc/quaternion.hpp"

#include <vector>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

constexpr float aspectRatio = (float)SCREEN_WIDTH / SCREEN_HEIGHT;

struct Plane {
	glm::vec3 direction;
	glm::vec3 center;
};

struct ZXMaterial {
	glm::vec3 color;
	float roughness;
};

struct Wall {
	unsigned int materialId = 0;
	int portal = -1;
};

struct Sector {
	std::vector<glm::vec3> boundary;
	std::vector<Wall> walls;
	std::vector<Plane> wallPlanes;
	std::vector<ZXMaterial> materials;

	Wall topInfo;
	Wall floorInfo;
	Plane bottom;
	Plane top;

	glm::vec3 GetPoint(unsigned int index)
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
			wall.direction = normal;

			left.center = p0;
			left.direction = dir;

			right.center = p1;
			right.direction = -dir;

			wallPlanes.push_back(left);
			wallPlanes.push_back(wall);
			wallPlanes.push_back(right);
		}
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

	void SetFOV(float fov) {
		fieldOfView = glm::tan(glm::radians(fieldOfView * 0.5f));
	}

	ZXRay ScreenPointToRay(float x, float y)
	{
		float px = (2.0f * (x + 0.5f) / SCREEN_WIDTH - 1.0f) * fieldOfView * aspectRatio;
		float py = (1.0f - 2.0f * (y + 0.5f) / SCREEN_HEIGHT) * fieldOfView;

		glm::vec4 rayPos = transform.transform * glm::vec4(px, py, -1.0f, 1.0f);

		ZXRay ray;
		ray.origin = transform.position;
		ray.direction = glm::vec3(rayPos) - transform.position;
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
		//sector->GetBoundaryPlanes(i, planes[0], planes[1], planes[2]);
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

void SetPixel(int x, int y, glm::vec3 color, unsigned char* screenbuffer) 
{
	screenbuffer[4 * (x + y * SCREEN_WIDTH) + 0] = 255 * color.r;
	screenbuffer[4 * (x + y * SCREEN_WIDTH) + 1] = 255 * color.g;
	screenbuffer[4 * (x + y * SCREEN_WIDTH) + 2] = 255 * color.b;
	screenbuffer[4 * (x + y * SCREEN_WIDTH) + 3] = 255;
}

glm::vec3 ComputeDirectLighting(glm::vec3 position, glm::vec3 normal, ZXMaterial& material)
{
	glm::vec3 lightPos(0, 2, 0);
	glm::vec3 lightDir = position - lightPos;
	float nDotL = glm::dot(lightDir, normal);
	return material.color * 2.0f * nDotL;
}

void Render(Sector* sector, ZXCamera* camera, unsigned char* screenbuffer)
{
	camera->transform.localToWorld();
	ZXRay ray;


	for (int y = 0; y < SCREEN_HEIGHT; y++)
	{
		for (int x = 0; x < SCREEN_WIDTH; x++)
		{
			ZXRay ray = camera->ScreenPointToRay(x, y);
			screenbuffer[4 * (x + y * SCREEN_WIDTH) + 0] = 0;
			screenbuffer[4 * (x + y * SCREEN_WIDTH) + 1] = 0;
			screenbuffer[4 * (x + y * SCREEN_WIDTH) + 2] = 0;
			screenbuffer[4 * (x + y * SCREEN_WIDTH) + 3] = 255;

			for (int i = 0; i < sector->boundary.size(); i++)
			{
				Plane left = sector->wallPlanes[3 * i];
				Plane wall = sector->wallPlanes[3 * i + 1];
				Plane right = sector->wallPlanes[3 * i + 2];
				float t = -1;
				if (RayPlaneIntersection(ray, wall, t))
				{
					glm::vec3 hitPoint = ray.At(t);

					if (SideOf(hitPoint, left) > 0 &&
						SideOf(hitPoint, right) > 0 &&
						SideOf(hitPoint, sector->top) > 0 &&
						SideOf(hitPoint, sector->bottom) > 0)
					{
						glm::vec3 color = ComputeDirectLighting(hitPoint, wall.direction, sector->materials[0]);
						SetPixel(x, y, color, screenbuffer);
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

#include <chrono>
class ScopedProfiler {
public:
	ScopedProfiler() {
		this->m_start = std::chrono::high_resolution_clock::now();
	}

	~ScopedProfiler() {
		const auto end = std::chrono::high_resolution_clock::now();

		std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count() << "ms" << std::endl;
	}

private:
	std::chrono::high_resolution_clock::time_point m_start;
};

class FPSProfiler {
public:
	FPSProfiler() {
		this->m_start = std::chrono::high_resolution_clock::now();
	}

	~FPSProfiler() {
		const auto end = std::chrono::high_resolution_clock::now();

		std::cout << 1000.0 / std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count() << "fps" << std::endl;
	}

private:
	std::chrono::high_resolution_clock::time_point m_start;
};

void main()
{
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "ZX");
	//SetTargetFPS(60);

	ZXCamera camera;
	//camera.transform.position = glm::vec3(0, 0, 0);
	Sector sector;
	sector.bottom.center = glm::vec3(0, -1, 0);
	sector.bottom.direction = glm::vec3(0, 1, 0);
	sector.top.center = glm::vec3(0, 1, 0);
	sector.top.direction = glm::vec3(0, -1, 0);
	sector.materials.push_back({
		{1, 1, 0},
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
	sector.boundary.push_back(glm::vec3(-30, 0, 0));

	Image image = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, RAYWHITE);
	Texture2D texture = LoadTextureFromImage(image);

	unsigned char* screenBuffer = new unsigned char[4 * SCREEN_WIDTH * SCREEN_HEIGHT];

	camera.SetFOV(60);

	sector.ComputePlanes();

	while (!WindowShouldClose())
	{
		FPSProfiler profiler;
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
		Render(&sector, &camera, screenBuffer);
		DrawMinimap(&camera, &sector);
		UpdateTexture(texture, screenBuffer);
		DrawTexture(texture, 0, 0, WHITE);
		EndDrawing();
	}

	CloseWindow();
}

