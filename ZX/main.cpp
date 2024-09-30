#include <iostream>
#include "raylib.h"
#include "glm/glm.hpp"
#include "glm/common.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/euler_angles.hpp"
#include <vector>

const int RENDER_WIDTH = 640;
const int RENDER_HEIGHT = 480;

constexpr float aspectRatio = (float)RENDER_WIDTH / RENDER_HEIGHT;

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
		/*matrix = glm::rotate(matrix, rotation.x, glm::vec3(1, 0, 0));
		matrix = glm::rotate(matrix, rotation.y, glm::vec3(0, 1 ,0));
		matrix = glm::rotate(matrix, rotation.z, glm::vec3(0, 0, 1));*/

		matrix = matrix * toQuaternion();

		this->transform = matrix;
		return matrix;
	}

	glm::vec3 Forward()
	{
		return toQuaternion() * glm::vec4(0, 0, -1, 0);
	}

	glm::vec3 Right()
	{
		return toQuaternion() * glm::vec4(1, 0, 0, 0);
	}

private:
	glm::mat4 toQuaternion() const
	{
		return  glm::orientate4(this->rotation);
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
		float px = (2.0f * (x + 0.5f) / RENDER_WIDTH - 1.0f) * fieldOfView * aspectRatio;
		float py = (1.0f - 2.0f * (y + 0.5f) / RENDER_HEIGHT) * fieldOfView;

		glm::vec4 rayPos = transform.transform * glm::vec4(px, py, -1.0f, 1.0f);

		ZXRay ray;
		ray.origin = transform.position;
		ray.direction = glm::vec3(rayPos) - transform.position;
		ray.direction = glm::normalize(ray.direction);
		return ray;
	}
};

bool RayPlaneIntersection(ZXRay ray, Plane p, float* t)
{
	float d = glm::dot(p.direction, ray.direction);
	if (glm::abs(d) > 0.0001f)
	{
		*t = glm::dot(p.direction, p.center - ray.origin) / d;
		return *t >= 0;
	}
	return false;
}

float SideOf(glm::vec3 point, Plane p)
{
	return glm::dot(point - p.center, p.direction);
}

struct RaycastHit {
	float t;
	glm::vec3 normal;
	glm::vec3 point;
};

void SetPixel(int x, int y, glm::vec3 color, unsigned char* screenbuffer) 
{
	color = glm::saturate(color);
	screenbuffer[4 * (x + y * RENDER_WIDTH) + 0] = 255 * color.r;
	screenbuffer[4 * (x + y * RENDER_WIDTH) + 1] = 255 * color.g;
	screenbuffer[4 * (x + y * RENDER_WIDTH) + 2] = 255 * color.b;
	screenbuffer[4 * (x + y * RENDER_WIDTH) + 3] = 255;
}

glm::vec3 ComputeDirectLighting(glm::vec3 position, glm::vec3 normal, ZXMaterial& material)
{
	glm::vec3 lightPos(10, 1, 10);
	glm::vec3 lightDir = lightPos - position;
	float distance = glm::inversesqrt(glm::dot(lightDir, lightDir));

	float nDotL = glm::clamp(glm::dot(glm::normalize(lightDir), normal), 0.0f, 1.0f);
	
	glm::vec3 color = (glm::vec3(1, 1, 1) + normal * 0.1f) * 0.7f;
	return 10.0f * (distance) * color;
}

bool RaycastSector(Sector* sector, ZXRay ray, RaycastHit& hit)
{
	for (int i = 0; i < sector->boundary.size(); i++)
	{
		Plane& left = sector->wallPlanes[3 * i];
		Plane& wall = sector->wallPlanes[3 * i + 1];
		Plane& right = sector->wallPlanes[3 * i + 2];
		if (RayPlaneIntersection(ray, wall, &hit.t))
		{
			hit.point = ray.At(hit.t);

			if (SideOf(hit.point, left) > 0 &&
				SideOf(hit.point, right) > 0 &&
				SideOf(hit.point, sector->top) > 0 &&
				SideOf(hit.point, sector->bottom) > 0)
			{
				hit.normal = wall.direction;
				return true;
			}
		}
	}


	// Need to make sure we pick the right intersection
	float topDistance = 0, bottomDistance = 0;
	bool hitFloorOrCeil = false;
	if (RayPlaneIntersection(ray, sector->top, &topDistance))
	{
		hit.point = ray.At(topDistance);
		hit.normal = sector->top.direction;
		hitFloorOrCeil = true;
	}
	if (RayPlaneIntersection(ray, sector->bottom, &bottomDistance))
	{
		if (!hitFloorOrCeil || glm::distance(ray.origin, ray.At(bottomDistance)) < glm::distance(ray.origin, ray.At(topDistance)))
		{
			hit.point = ray.At(bottomDistance);
			hit.normal = sector->bottom.direction;
			hitFloorOrCeil = true;
		}
	}
	return hitFloorOrCeil;
}

std::vector<glm::vec3> s_randomOnUnitSphere;
size_t randomCounter = 0;


glm::vec3 IndirectLight(Sector* sector, ZXRay ray)
{
	RaycastHit hit;
	glm::vec3 direct(0.0f, 0.0f, 0.0f);
	if (RaycastSector(sector, ray, hit))
	{
		direct = ComputeDirectLighting(hit.point, hit.normal, sector->materials[0]);
	}

	return direct;
}

void Render(Sector* sector, ZXCamera* camera, unsigned char* screenbuffer)
{
	camera->transform.localToWorld();
	ZXRay ray;


	for (int y = 0; y < RENDER_HEIGHT; y++)
	{
		for (int x = 0; x < RENDER_WIDTH; x++)
		{
			ZXRay ray = camera->ScreenPointToRay(x, y);
			glm::vec3 color = IndirectLight(sector, ray);
			SetPixel(x, y, color, screenbuffer);
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

float RandomFloat(float min, float max)
{
	float t = (float)GetRandomValue(0, 10000) / 10000.0f;
	return min + (max - min) * t;
}

void main()
{
	InitWindow(RENDER_WIDTH, RENDER_HEIGHT, "ZX");
	SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);

	int screenWidth = GetScreenWidth();
	int screenHeight = GetScreenHeight();

	// MaximizeWindow();
	SetTargetFPS(60);

	ZXCamera camera;
	//camera.transform.position = glm::vec3(0, 0, 0);
	Sector sector;
	sector.bottom.center = glm::vec3(0.0f, 0.0f, 0.0f);
	sector.bottom.direction = glm::normalize(glm::vec3(0.0f, 1.0f, -0.1f));

	sector.top.center = glm::vec3(0.0f, 5.0f, 0.0f);
	sector.top.direction = glm::normalize(glm::vec3(0, -1.0f, 0.0f));
	sector.materials.push_back({
		{1, 1, 0},
		1.0f
	});

	sector.materials.push_back({
		{1.0f, 1.0f, 0.5f},
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

	Image image = GenImageColor(RENDER_WIDTH, RENDER_HEIGHT, RAYWHITE);
	Texture2D texture = LoadTextureFromImage(image);

	unsigned char* screenBuffer = new unsigned char[4 * RENDER_WIDTH * RENDER_HEIGHT];
	bool cursorHidden = true;

	HideCursor();
	DisableCursor();

	camera.SetFOV(60);

	sector.ComputePlanes();

	for (int i = 0; i < 10; i++)
	{
		glm::vec3 dir = glm::vec3(RandomFloat(-10, 10), RandomFloat(-10, 10), RandomFloat(-10, 10));
		s_randomOnUnitSphere.push_back(glm::normalize(dir));
	}

	Rectangle srcRect{ 0, 0, RENDER_WIDTH, RENDER_HEIGHT };
	Rectangle dstRect{ 0, 0, screenWidth, screenHeight };


	while (!WindowShouldClose())
	{
		FPSProfiler profiler;
		float dt = 1 / 60.0;

		if (IsKeyDown(KEY_A))
		{
			camera.transform.position -= camera.transform.Right() * 10.0f * dt;
		}
		if (IsKeyDown(KEY_D))
		{
			camera.transform.position += camera.transform.Right() * 10.0f * dt;
		}
		if (IsKeyDown(KEY_W))
		{
			camera.transform.position += camera.transform.Forward() * 10.0f * dt;
		}
		if (IsKeyDown(KEY_S))
		{
			camera.transform.position -= camera.transform.Forward() * 10.0f * dt;
		}
		if (IsKeyDown(KEY_Q)) 
		{
			if (cursorHidden)
			{
				ShowCursor();
				EnableCursor();
			}
			else 
			{
				HideCursor();
				DisableCursor();
			}
		}

		Vector2 mouseDelta = GetMouseDelta();
		camera.transform.rotation.z -= mouseDelta.x * 0.1f * dt;
		camera.transform.rotation.x -= mouseDelta.y * 0.1f * dt;

		//std::cout << camera.transform.position.x << ", " << camera.transform.position.y << ", " << camera.transform.position.z << std::endl;

		BeginDrawing();
		ClearBackground(RAYWHITE);
		Render(&sector, &camera, screenBuffer);
		UpdateTexture(texture, screenBuffer);
		DrawTexturePro(texture, srcRect, dstRect, {}, 0, WHITE);
		DrawMinimap(&camera, &sector);
		EndDrawing();
	}

	CloseWindow();
}

