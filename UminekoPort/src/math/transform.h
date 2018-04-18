#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct Transform {
	glm::vec3 position;
	glm::vec3 scale = glm::vec3(1, 1, 1);
	glm::quat rotation;
};