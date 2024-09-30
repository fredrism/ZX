#pragma once
#include "../math/Ray.h"
#include "Transform.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
constexpr float aspectRatio = (float)SCREEN_WIDTH / SCREEN_HEIGHT;

class Camera
{
public:
	Camera();
	~Camera();

	void SetFOV(float degrees);
	float GetFOV() const;
	Transform& GetTransform();

	Ray ScreenPointToRay(float x, float y) const;

private:
	float m_fieldOfView;
	float m_rayOffset;
	Transform m_transform;
};


struct ZXCamera
{
	
};