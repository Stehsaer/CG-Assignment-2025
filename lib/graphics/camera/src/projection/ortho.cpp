#include "graphics/camera/projection/ortho.hpp"

#include <glm/gtc/matrix_transform.hpp>

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#error "GLM_FORCE_DEPTH_ZERO_TO_ONE must be defined"
#endif

namespace graphics::camera::projection
{
	glm::dmat4 Ortho::matrix(float aspect_ratio) const noexcept
	{
		const auto viewport_width = viewport_height * aspect_ratio;
		return glm::ortho<double>(
			-viewport_width / 2,
			viewport_width / 2,
			-viewport_height / 2,
			viewport_height / 2,
			near_plane,
			far_plane
		);
	}

	glm::dmat4 OrthoFixed::matrix(float aspect_ratio [[maybe_unused]]) const noexcept
	{
		return glm::ortho<double>(
			-viewport_size.x / 2,
			viewport_size.x / 2,
			-viewport_size.y / 2,
			viewport_size.y / 2,
			near_plane,
			far_plane
		);
	}
}