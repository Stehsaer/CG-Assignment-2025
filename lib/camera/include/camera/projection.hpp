#pragma once

#include <glm/ext/matrix_double4x4.hpp>

namespace camera
{
	class View;

	///
	/// @brief Interface for a camera projection
	///
	///
	class Projection
	{
	  public:

		virtual ~Projection() = default;

		///
		/// @brief Acquire the projection matrix, far plane at 1, near plane at 0
		///
		/// @param aspect_ratio Aspect ratio of input viewport, calculated by `width / height`
		/// @return Projection matrix
		///
		virtual glm::dmat4 matrix(float aspect_ratio) noexcept = 0;

		///
		/// @brief Acquire the projection matrix with reversed Z-axis
		///
		/// @param aspect_ratio Aspect ratio of input viewport, calculated by `width / height`
		/// @return Projection matrix with reversed Z-axis
		///
		glm::dmat4 matrix_reverse_z(float aspect_ratio) noexcept;
	};
}