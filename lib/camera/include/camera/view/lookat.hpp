#pragma once

#include "camera/view.hpp"

#include <glm/vec3.hpp>

namespace camera::view
{
	///
	/// @brief A view that looks at a target point
	///
	///
	struct Lookat : public View
	{
		Lookat(const Lookat&) = default;
		Lookat(Lookat&&) = default;
		Lookat& operator=(const Lookat&) = default;
		Lookat& operator=(Lookat&&) = default;

		///
		/// @brief Create a look-at view
		///
		/// @param eye Eye position
		/// @param center Target position
		/// @param up Up direction
		///
		Lookat(glm::vec3 eye, glm::vec3 center, glm::vec3 up) noexcept;

		virtual ~Lookat() = default;

		glm::vec3 eye;
		glm::vec3 center;
		glm::vec3 up;

		glm::dmat4 matrix() noexcept override;
	};
}