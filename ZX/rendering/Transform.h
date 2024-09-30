#pragma once
#include "glm/glm.hpp"
#include "glm/glm.hpp"
#include "glm/common.hpp"
#include "glm/gtc/quaternion.hpp"

struct Transform
{
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
	glm::mat4 transform;

	Transform()
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
		matrix = glm::rotate(matrix, rotation.y, glm::vec3(0, 1, 0));
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