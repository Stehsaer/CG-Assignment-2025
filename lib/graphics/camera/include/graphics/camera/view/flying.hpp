#pragma once

#include <glm/glm.hpp>

#include "graphics/camera/spherical-angle.hpp"

namespace graphics::camera::view
{
	struct Flying
	{
		glm::dvec3 position;
		SphericalAngle angles;
		glm::dvec3 up;

		glm::dmat4 matrix() const noexcept;
		glm::dvec3 eye_position() const noexcept;

		static Flying lerp(const Flying& a, const Flying& b, double t) noexcept;

		Flying move(this const Flying& self, glm::dvec3 delta) noexcept;
	};
}