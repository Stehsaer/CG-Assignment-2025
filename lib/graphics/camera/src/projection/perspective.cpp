#include "graphics/camera/projection/perspective.hpp"

#include <glm/gtc/matrix_transform.hpp>

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#error "GLM_FORCE_DEPTH_ZERO_TO_ONE must be defined"
#endif

namespace graphics::camera::projection
{
	glm::dmat4 Perspective::matrix(float aspect_ratio) const noexcept
	{
		if (far_plane.has_value())
			return glm::perspective<double>(fov_y, aspect_ratio, near_plane, far_plane.value());
		else
			return glm::infinitePerspective<double>(fov_y, aspect_ratio, near_plane);
	}

	glm::dmat4 Perspective::matrix_reverse_z(float aspect_ratio) const noexcept
	{
		const static glm::dmat4 reversez_mat = glm::dmat4(
			glm::dvec4(1, 0, 0, 0),
			glm::dvec4(0, 1, 0, 0),
			glm::dvec4(0, 0, -1, 0),
			glm::dvec4(0, 0, 1, 1)
		);

		return reversez_mat * matrix(aspect_ratio);
	}
}