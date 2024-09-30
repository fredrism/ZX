#include "Renderer.h"
#include "glm/glm.hpp"
#include "glm/common.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/euler_angles.hpp"

Renderer::Renderer(uint32_t width, uint32_t height)
	: m_width(width), m_height(height)
{
	m_colorBuffer = new unsigned char[width * height * 4];
	m_depthBuffer = new float[width * height];

	m_image = RL::GenImageColor(width, height, RL::RAYWHITE);
	m_texture = RL::LoadTextureFromImage(m_image);
}

Renderer::~Renderer()
{
	RL::UnloadImage(m_image);
	RL::UnloadTexture(m_texture);
	delete[] m_colorBuffer;
	delete[] m_depthBuffer;
}

glm::vec3 Renderer::IndirectLight(const Sector& sector, const Ray& ray)
{
	RaycastHit hit;
	glm::vec3 direct(0.0f, 0.0f, 0.0f);
	if (sector.Raycast(ray, hit))
	{
		direct = ComputeDirectLighting(hit.point, hit.normal, sector.materials[0]);
	}

	return direct;
}

void Renderer::Render(const Sector& sector, Camera& camera)
{
	camera.GetTransform().localToWorld();
	
	for (int y = 0; y < m_height; y++)
	{
		for (int x = 0; x < m_width; x++)
		{
			Ray ray = camera.ScreenPointToRay(x, y);
			glm::vec3 color = IndirectLight(sector, ray);
			SetPixel(x, y, color, 0);
		}
	}
}

glm::vec3 Renderer::ComputeDirectLighting(glm::vec3 position, glm::vec3 normal, const Material& material)
{
	glm::vec3 lightPos(10, 1, 10);
	glm::vec3 lightDir = lightPos - position;
	float distance = glm::inversesqrt(glm::dot(lightDir, lightDir));

	float nDotL = glm::clamp(glm::dot(glm::normalize(lightDir), normal), 0.0f, 1.0f);

	glm::vec3 color = (glm::vec3(1, 1, 1) + normal * 0.1f) * 0.7f;
	return 10.0f * (distance)*color;
}

void Renderer::FinishDrawing(uint32_t screenWidth, uint32_t screenHeight)
{
	RL::Rectangle srcRect{ 0, 0, m_width, m_height };
	RL::Rectangle dstRect{ 0, 0, screenWidth, screenHeight };

	RL::BeginDrawing();
	RL::UpdateTexture(m_texture, m_colorBuffer);
	RL::DrawTexturePro(m_texture, srcRect, dstRect, {}, 0, RL::WHITE);
	RL::EndDrawing();
}

void Renderer::SetPixel(int x, int y, glm::vec3 color, float depth)
{
	color = glm::saturate(color);
	m_colorBuffer[4 * (x + y * m_width) + 0] = 255 * color.r;
	m_colorBuffer[4 * (x + y * m_width) + 1] = 255 * color.g;
	m_colorBuffer[4 * (x + y * m_width) + 2] = 255 * color.b;
	m_colorBuffer[4 * (x + y * m_width) + 3] = 255;
}