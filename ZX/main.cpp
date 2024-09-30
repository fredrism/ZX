#include <iostream>
#include "level/Sector.h"
#include "rendering/Renderer.h"
#include "rendering/Camera.h"

namespace RL
{
#include "raylib.h"
}

/*void DrawMinimap(ZXCamera* camera, Sector* sector)
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
}*/

void main()
{
	RL::InitWindow(640, 480, "ZX");
	RL::SetWindowState(RL::FLAG_WINDOW_RESIZABLE | RL::FLAG_WINDOW_MAXIMIZED);

	int screenWidth = RL::GetScreenWidth();
	int screenHeight = RL::GetScreenHeight();

	// MaximizeWindow();
	RL::SetTargetFPS(60);

	Camera camera;
	//camera.transform.position = glm::vec3(0, 0, 0);
	Sector sector;
	sector.bottom.center = glm::vec3(0.0f, 0.0f, 0.0f);
	sector.bottom.normal = glm::normalize(glm::vec3(0.0f, 1.0f, -0.1f));

	sector.top.center = glm::vec3(0.0f, 5.0f, 0.0f);
	sector.top.normal = glm::normalize(glm::vec3(0, -1.0f, 0.0f));
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

	Renderer renderer(640, 480);

	bool cursorHidden = true;

	RL::HideCursor();
	RL::DisableCursor();

	camera.SetFOV(60);
	sector.ComputePlanes();

	while (!RL::WindowShouldClose())
	{
		float dt = 1 / 60.0;

		if (RL::IsKeyDown(RL::KEY_A))
		{
			camera.GetTransform().position -= camera.GetTransform().Right() * 10.0f * dt;
		}
		if (RL::IsKeyDown(RL::KEY_D))
		{
			camera.GetTransform().position += camera.GetTransform().Right() * 10.0f * dt;
		}
		if (RL::IsKeyDown(RL::KEY_W))
		{
			camera.GetTransform().position += camera.GetTransform().Forward() * 10.0f * dt;
		}
		if (RL::IsKeyDown(RL::KEY_S))
		{
			camera.GetTransform().position -= camera.GetTransform().Forward() * 10.0f * dt;
		}
		if (RL::IsKeyDown(RL::KEY_Q))
		{
			if (cursorHidden)
			{
				RL::ShowCursor();
				RL::EnableCursor();
			}
			else 
			{
				RL::HideCursor();
				RL::DisableCursor();
			}
		}

		RL::Vector2 mouseDelta = RL::GetMouseDelta();
		camera.GetTransform().rotation.z -= mouseDelta.x * 0.1f * dt;
		camera.GetTransform().rotation.x -= mouseDelta.y * 0.1f * dt;

		renderer.Render(sector, camera);
		renderer.FinishDrawing(screenWidth, screenHeight);
	}

	RL::CloseWindow();
}

