#include "graphics/slerp.hpp"

namespace graphics
{
	glm::vec3 slerp(const glm::vec3& a, const glm::vec3& b, float t) noexcept
	{
		const glm::vec3 u = glm::normalize(a);
		const glm::vec3 v = glm::normalize(b);
		const float dot = glm::clamp(glm::dot(u, v), -1.0f, 1.0f);
		const float theta = glm::acos(dot);

		if (theta < 1e-6f) return glm::mix(u, v, t);

		const float sinTheta = glm::sin(theta);
		const float coeff1 = glm::sin((1.0f - t) * theta) / sinTheta;
		const float coeff2 = glm::sin(t * theta) / sinTheta;

		return coeff1 * u + coeff2 * v;
	}
}