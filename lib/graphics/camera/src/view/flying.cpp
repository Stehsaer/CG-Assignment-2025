#include "graphics/camera/view/flying.hpp"
#include "graphics/slerp.hpp"

#include <glm/ext/matrix_transform.hpp>

namespace graphics::camera::view
{
	glm::dmat4 Flying::matrix() const noexcept
	{
		const auto front = angles.facing();
		return glm::lookAt(position, position + front, up);
	}

	glm::dvec3 Flying::eye_position() const noexcept
	{
		return position;
	}

	Flying Flying::lerp(const Flying& a, const Flying& b, double t) noexcept
	{
		return Flying{
			.position = glm::mix(a.position, b.position, t),
			.angles = SphericalAngle::lerp(a.angles, b.angles, t),
			.up = graphics::slerp(a.up, b.up, t)
		};
	}

	Flying Flying::move(this const Flying& self, glm::dvec3 delta) noexcept
	{
		const auto inv_matrix = glm::inverse(self.matrix());
		const auto view_space_delta_homo = glm::dvec4(delta, 0.0);
		const auto world_space_delta_homo = inv_matrix * view_space_delta_homo;
		const auto world_space_delta = glm::dvec3(world_space_delta_homo);

		return {.position = self.position + world_space_delta, .angles = self.angles, .up = self.up};
	}
}