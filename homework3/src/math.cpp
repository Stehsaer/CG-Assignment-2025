#include "math.hpp"

namespace math
{
	glm::vec2 world_to_uv(
		const glm::vec2& world_pos,
		const glm::mat4& vp_matrix
	) noexcept
	{
		const auto ndc_h = vp_matrix * glm::vec4(world_pos, 0.0f, 1.0f);
		const auto ndc = glm::vec2(ndc_h) / ndc_h.w;
		const auto uv = ndc * glm::vec2(0.5f, -0.5f) + glm::vec2(0.5f, 0.5f);
		return uv;
	}

	glm::vec2 uv_to_world(
		const glm::vec2& uv,
		const glm::mat4& vp_matrix
	) noexcept
	{
		const auto ndc = uv * glm::vec2(2.0f, -2.0f) + glm::vec2(-1.0f, 1.0f);
		const auto inv_vp_matrix = glm::inverse(vp_matrix);
		const auto world_h = inv_vp_matrix * glm::vec4(ndc, 0.0f, 1.0f);
		return glm::vec2(world_h) / world_h.w;
	}
}