#include "graphics/camera/view/orbit.hpp"

#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace graphics::camera::view
{
	glm::dmat4 Orbit::matrix() const noexcept
	{
		const glm::vec3 eye = this->eye_position();
		return glm::lookAt(eye, center, up);
	}

	glm::vec3 Orbit::eye_position() const noexcept
	{
		return {
			center.x + distance * glm::cos(pitch) * glm::sin(azimuth),
			center.y + distance * glm::sin(pitch),
			center.z + distance * glm::cos(pitch) * glm::cos(azimuth)
		};
	}

	void Orbit::Pan_controller::pan(Orbit& orbit, glm::vec2 screen_size, glm::vec2 pixel_delta) const noexcept
	{
		if (screen_size.x < 1 || screen_size.y < 1) return;

		const auto distance_per_pixel = conversion_factor * orbit.distance * 2 / screen_size.y;
		const auto inv_matrix = glm::inverse(orbit.matrix());
		const auto view_space_delta = glm::vec3(-pixel_delta.x, pixel_delta.y, 0.0f) * distance_per_pixel;
		const auto view_space_delta_homo = glm::vec4(view_space_delta, 0.0f);
		const auto world_space_delta_homo = inv_matrix * view_space_delta_homo;
		const auto world_space_delta = glm::vec3(world_space_delta_homo);

		orbit.center += world_space_delta;
	}

	void Orbit::Rotate_controller::rotate(
		Orbit& orbit,
		glm::vec2 screen_size,
		glm::vec2 pixel_delta
	) const noexcept
	{
		if (screen_size.x < 1 || screen_size.y < 1) return;

		const auto azimuth_per_pixel = azimuth_per_width / screen_size.x;
		const auto pitch_per_pixel = pitch_per_height / screen_size.y;
		const auto azimuth_delta = -pixel_delta.x * azimuth_per_pixel;
		const auto pitch_delta = pixel_delta.y * pitch_per_pixel;

		orbit.azimuth += azimuth_delta;
		orbit.pitch += pitch_delta;

		orbit.pitch = glm::clamp(orbit.pitch, -glm::pi<float>() / 2 + 0.001f, glm::pi<float>() / 2 - 0.001f);
	}
}