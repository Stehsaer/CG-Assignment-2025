#pragma once

#include <glm/glm.hpp>

namespace graphics::camera
{
	struct SphericalAngle
	{
		double azimuth;
		double pitch;

		glm::dvec3 facing(this SphericalAngle self) noexcept;

		static SphericalAngle lerp(const SphericalAngle& a, const SphericalAngle& b, double t) noexcept;

		SphericalAngle rotate(
			this SphericalAngle self,
			float azimuth_per_width,
			float pitch_per_height,
			glm::vec2 screen_size,
			glm::vec2 pixel_delta
		) noexcept;
	};
}