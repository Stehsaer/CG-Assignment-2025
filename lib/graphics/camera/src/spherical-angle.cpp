#include "graphics/camera/spherical-angle.hpp"

#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/constants.hpp>

namespace graphics::camera
{
	glm::dvec3 SphericalAngle::facing(this SphericalAngle self) noexcept
	{
		return {
			glm::cos(self.pitch) * glm::sin(self.azimuth),
			glm::sin(self.pitch),
			glm::cos(self.pitch) * glm::cos(self.azimuth)
		};
	}

	SphericalAngle SphericalAngle::lerp(const SphericalAngle& a, const SphericalAngle& b, double t) noexcept
	{
		const auto azimuth_a = glm::mod(a.azimuth, glm::two_pi<double>());
		const auto azimuth_b = glm::mod(b.azimuth, glm::two_pi<double>());

		double azimuth_diff = glm::mod(azimuth_b + glm::two_pi<double>() - azimuth_a, glm::two_pi<double>());
		if (azimuth_diff > glm::pi<double>()) azimuth_diff -= glm::two_pi<double>();

		const auto interpolated_azimuth = glm::mix(a.azimuth, a.azimuth + azimuth_diff, t);

		return {
			.azimuth = glm::mod(interpolated_azimuth, glm::two_pi<double>()),
			.pitch = glm::mix(a.pitch, b.pitch, t)
		};
	}

	SphericalAngle SphericalAngle::rotate(
		this SphericalAngle self,
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
			.azimuth = fmod(self.azimuth + azimuth_delta, glm::two_pi<double>()),
			.pitch = glm::clamp(
				self.pitch + pitch_delta,
				-glm::pi<double>() / 2 + 0.001,
				glm::pi<double>() / 2 - 0.001
			)
		};
	}
}