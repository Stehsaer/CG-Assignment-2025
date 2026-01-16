#pragma once

#include <glm/glm.hpp>

namespace math
{
	glm::vec2 world_to_uv(const glm::vec2& world_pos, const glm::mat4& vp_matrix) noexcept;

	glm::vec2 uv_to_world(const glm::vec2& uv, const glm::mat4& vp_matrix) noexcept;
}