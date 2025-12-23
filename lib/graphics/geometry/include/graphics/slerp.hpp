#pragma once

#include <glm/glm.hpp>

namespace graphics
{
	glm::vec3 slerp(const glm::vec3& a, const glm::vec3& b, float t) noexcept;
}