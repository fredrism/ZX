#pragma once
#include "Camera.h"
#include "../math/Ray.h"
#include "../level/Sector.h"
namespace RL
{
#include "raylib.h"
}

class Renderer
{
public:
	Renderer(uint32_t width, uint32_t height);
	~Renderer();
	
	void Render(const Sector& sector, Camera& camera);
	void FinishDrawing(uint32_t screenWidth, uint32_t screenHeight);

private:
	glm::vec3 IndirectLight(const Sector& sector, const Ray& ray);
	glm::vec3 ComputeDirectLighting(glm::vec3 position, glm::vec3 normal, const Material& material);
	void SetPixel(int x, int y, glm::vec3 color, float depth);
	
	uint32_t m_width;
	uint32_t m_height;

	RL::Image m_image;
	RL::Texture2D m_texture;
	unsigned char* m_colorBuffer;
	float* m_depthBuffer;
};

