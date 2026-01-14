#pragma once

#include "graphics/camera/spherical-angle.hpp"

#include <glm/glm.hpp>

namespace graphics::camera::view
{
	///
	/// @brief Orbiting View
	///
	///
	struct Orbit
	{
		double distance;
		SphericalAngle angles;
		glm::dvec3 center;
		glm::dvec3 up;

		glm::dmat4 matrix() const noexcept;
		glm::dvec3 eye_position() const noexcept;

		static Orbit lerp(const Orbit& a, const Orbit& b, double t) noexcept;

		Orbit pan(
			this const Orbit& self,
			float conversion_factor,
			glm::vec2 screen_size,
			glm::vec2 pixel_delta
		) noexcept;
	};
}