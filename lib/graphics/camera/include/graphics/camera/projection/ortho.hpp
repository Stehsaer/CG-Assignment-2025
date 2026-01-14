#pragma once

#include <glm/glm.hpp>

namespace graphics::camera::projection
{
	///
	/// @brief Orthographic Projection, with view cube's aspect ratio matching the viewport's
	///
	///
	struct Ortho
	{
		float viewport_height;
		float near_plane;
		float far_plane;

		glm::dmat4 matrix(float aspect_ratio) const noexcept;
	};

	///
	/// @brief Fixed-aspect Orthographic Projection
	///
	struct OrthoFixed
	{
		glm::vec2 viewport_size;
		float near_plane;
		float far_plane;

		glm::dmat4 matrix(float aspect_ratio) const noexcept;
	};
}
