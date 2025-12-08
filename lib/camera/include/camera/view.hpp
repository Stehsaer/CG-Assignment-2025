#pragma once

#include <glm/ext/matrix_double4x4.hpp>

namespace camera
{
	class Projection;

	///
	/// @brief Interface for a camera view
	///
	///
	class View
	{
	  public:

		virtual ~View() = default;

		///
		/// @brief Get the view matrix
		///
		/// @return View matrix
		///
		virtual glm::dmat4 matrix() noexcept = 0;
	};
}