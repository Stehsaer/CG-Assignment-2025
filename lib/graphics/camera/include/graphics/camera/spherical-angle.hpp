#pragma once

#include <glm/glm.hpp>

namespace graphics::camera
{
	struct Spherical_angle
	{
		double azimuth;
		double pitch;

		glm::dvec3 facing(this Spherical_angle self) noexcept;

		static Spherical_angle lerp(const Spherical_angle& a, const Spherical_angle& b, double t) noexcept;

		Spherical_angle rotate(
			this Spherical_angle self,
			float azimuth_per_width,
			float pitch_per_height,
			glm::vec2 screen_size,
			glm::vec2 pixel_delta
		) noexcept;
	};
}