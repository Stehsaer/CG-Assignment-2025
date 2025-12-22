#pragma once

#include <glm/glm.hpp>
#include <optional>

namespace graphics::camera::projection
{
	///
	/// @brief Perspective Projection
	///
	///
	struct Perspective
	{
		float fov_y;
		float near_plane;
		std::optional<float> far_plane;  // if not set, use infinite projection

		///
		/// @brief Get perspective projection matrix
		///
		/// @param aspect_ratio Aspect ratio of the viewport (width / height)
		/// @return Projection matrix
		///
		glm::dmat4 matrix(float aspect_ratio) const noexcept;

		///
		/// @brief Get perspective projection matrix with reversed Z
		///
		/// @param aspect_ratio Aspect ratio of the viewport (width / height)
		/// @return Projection matrix with reversed Z
		///
		glm::dmat4 matrix_reverse_z(float aspect_ratio) const noexcept;
	};
}