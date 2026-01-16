#pragma once

#include <glm/glm.hpp>

struct Camera2D
{
	glm::vec2 center = {0.0f, 0.0f};
	float height = 1.0f;

	glm::mat4 get_matrix(this const Camera2D& self, float aspect_ratio) noexcept;

	void pan(this Camera2D& self, const glm::vec2& pixel_delta, const glm::vec2& viewport_size) noexcept;

	void zoom(
		this Camera2D& self,
		float zoom_factor,
		const glm::vec2& mouse_pos,
		const glm::vec2& viewport_size
	) noexcept;
};