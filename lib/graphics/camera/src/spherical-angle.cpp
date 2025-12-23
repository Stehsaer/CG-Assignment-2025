#include "graphics/camera/spherical-angle.hpp"

#include <glm/ext/scalar_constants.hpp>

namespace graphics::camera
{
	glm::dvec3 Spherical_angle::facing(this Spherical_angle self) noexcept
	{
		return {
			glm::cos(self.pitch) * glm::sin(self.azimuth),
			glm::sin(self.pitch),
			glm::cos(self.pitch) * glm::cos(self.azimuth)
		};
	}

	Spherical_angle Spherical_angle::lerp(
		const Spherical_angle& a,
		const Spherical_angle& b,
		double t
	) noexcept
	{
		return {.azimuth = glm::mix(a.azimuth, b.azimuth, t), .pitch = glm::mix(a.pitch, b.pitch, t)};
	}

	Spherical_angle Spherical_angle::rotate(
		this Spherical_angle self,
		float azimuth_per_width,
		float pitch_per_height,
		glm::vec2 screen_size,
		glm::vec2 pixel_delta
	) noexcept
	{
		if (screen_size.x < 1 || screen_size.y < 1) return self;

		const auto azimuth_per_pixel = azimuth_per_width / screen_size.x;
		const auto pitch_per_pixel = pitch_per_height / screen_size.y;
		const auto azimuth_delta = -pixel_delta.x * azimuth_per_pixel;
		const auto pitch_delta = pixel_delta.y * pitch_per_pixel;

		return {
			.azimuth = self.azimuth + azimuth_delta,
			.pitch = glm::clamp(
				self.pitch + pitch_delta,
				-glm::pi<double>() / 2 + 0.001,
				glm::pi<double>() / 2 - 0.001
			)
		};
	}
}