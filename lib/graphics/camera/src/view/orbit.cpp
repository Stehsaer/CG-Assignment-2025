#include "graphics/camera/view/orbit.hpp"
#include "graphics/slerp.hpp"

#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace graphics::camera::view
{
	glm::dmat4 Orbit::matrix() const noexcept
	{
		const auto eye = eye_position();
		return glm::lookAt(eye, center, up);
	}

	glm::dvec3 Orbit::eye_position() const noexcept
	{
		return center + distance * angles.facing();
	}

	Orbit Orbit::lerp(const Orbit& a, const Orbit& b, double t) noexcept
	{
		return Orbit{
			.distance = glm::mix(a.distance, b.distance, t),
			.angles = SphericalAngle::lerp(a.angles, b.angles, t),
			.center = glm::mix(a.center, b.center, t),
			.up = graphics::slerp(a.up, b.up, t)
		};
	}

	Orbit Orbit::pan(
		this const Orbit& self,
		float conversion_factor,
		glm::vec2 screen_size,
		glm::vec2 pixel_delta
	) noexcept
	{
		if (screen_size.x < 1 || screen_size.y < 1) return self;

		const auto distance_per_pixel = conversion_factor * self.distance * 2 / screen_size.y;
		const auto inv_matrix = glm::inverse(self.matrix());
		const auto view_space_delta = glm::dvec3(-pixel_delta.x, pixel_delta.y, 0.0) * distance_per_pixel;
		const auto view_space_delta_homo = glm::dvec4(view_space_delta, 0.0);
		const auto world_space_delta_homo = inv_matrix * view_space_delta_homo;
		const auto world_space_delta = glm::dvec3(world_space_delta_homo);

		return {
			.distance = self.distance,
			.angles = self.angles,
			.center = self.center + world_space_delta,
			.up = self.up
		};
	}
}