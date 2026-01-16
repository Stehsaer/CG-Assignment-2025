#include "logic/camera-2d.hpp"

#include <glm/ext/matrix_clip_space.hpp>

glm::mat4 Camera2D::get_matrix(this const Camera2D& self, float aspect_ratio) noexcept
{
	const float half_height = self.height * 0.5f;
	const float half_width = half_height * aspect_ratio;

	return glm::ortho(
		self.center.x - half_width,
		self.center.x + half_width,
		self.center.y - half_height,
		self.center.y + half_height,
		-1.0f,
		1.0f
	);
}

void Camera2D::pan(this Camera2D& self, const glm::vec2& pixel_delta, const glm::vec2& viewport_size) noexcept
{
	const float world_per_pixel = self.height / viewport_size.y;
	self.center -= pixel_delta * world_per_pixel;
}

void Camera2D::zoom(
	this Camera2D& self,
	float zoom_factor,
	const glm::vec2& mouse_pos,
	const glm::vec2& viewport_size
) noexcept
{
	const auto old_mat = self.get_matrix(viewport_size.x / viewport_size.y);
	const auto mouse_uv = mouse_pos / viewport_size;
	const auto mouse_ndc = mouse_uv * glm::vec2(2.0f, -2.0f) + glm::vec2(-1.0f, 1.0f);
	const auto mouse_world_h = glm::inverse(old_mat) * glm::vec4(mouse_ndc, 0.0f, 1.0f);
	const auto mouse_world = glm::vec2(mouse_world_h) / mouse_world_h.w;

	self.height *= zoom_factor;
	self.center = mouse_world + (self.center - mouse_world) * zoom_factor;
}
