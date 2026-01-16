#pragma once

#include <glm/glm.hpp>

namespace primitive
{
	struct ControlPoint
	{
		glm::vec2 position;
		bool dragging = false;

		bool operator==(const ControlPoint& other) const { return position == other.position; }

		void draw(const glm::mat4& vp_matrix) const noexcept;
		void drag(const glm::mat4& vp_matrix) noexcept;
	};
}