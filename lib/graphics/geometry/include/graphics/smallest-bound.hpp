#pragma once

#include <array>
#include <glm/glm.hpp>

namespace graphics
{
	struct SmallestBound
	{
		glm::mat4 view_matrix;
		float left, right, top, bottom;
	};

	SmallestBound find_smallest_bound(
		const std::array<glm::vec3, 8>& frustum_corners,
		const glm::vec3& view_dir
	) noexcept;
}