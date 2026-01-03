#include "logic/camera-control.hpp"
#include "graphics/camera/view/flying.hpp"
#include "render/param.hpp"

#include <SDL3/SDL_mouse.h>
#include <cassert>
#include <imgui.h>

namespace logic
{
	render::Camera_matrices Camera::update(const graphics::camera::view::Flying& camera) noexcept
	{
		const auto [width, height] = ImGui::GetIO().DisplaySize;
		const auto aspect_ratio = width / height;

		this->camera = graphics::camera::view::Flying::lerp(
			this->camera.value_or(camera),
			camera,
			glm::clamp(mix_factor * ImGui::GetIO().DeltaTime, 0.0f, 1.0f)
		);

		assert(this->camera.has_value());

		const auto view_matrix = this->camera->matrix();
		const auto proj_matrix = camera_projection.matrix_reverse_z(aspect_ratio);
		const auto camera_matrix = proj_matrix * view_matrix;
		const auto prev_matrix = prev_frame_camera_matrix.value_or(camera_matrix);
		prev_frame_camera_matrix = camera_matrix;

		return render::Camera_matrices{
			.view_matrix = view_matrix,
			.proj_matrix = proj_matrix,
			.prev_view_proj_matrix = prev_matrix,
			.eye_position = this->camera->eye_position(),
		};
	}

	graphics::camera::view::Flying Free_camera::update(
		const backend::SDL_context& context,
		bool free_cam
	) noexcept
	{
		auto& io = ImGui::GetIO();
		const auto [width, height] = io.DisplaySize;

		SDL_SetWindowRelativeMouseMode(
			context.window,
			!io.WantCaptureMouse && ImGui::IsMouseDown(ImGuiMouseButton_Right)
		);

		if (!io.WantCaptureMouse)
		{
			glm::vec2 mouse_delta;
			SDL_GetRelativeMouseState(&mouse_delta.x, &mouse_delta.y);

			if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
			{
				target_camera.angles = target_camera.angles.rotate(
					azimuth_per_width,
					pitch_per_height,
					{float(width), float(height)},
					mouse_delta * glm::vec2{1.0f, -1.0f}
				);
			}

			glm::dvec3 position_delta{0.0};
			double distance = ImGui::GetIO().DeltaTime * 2.0;

			if (ImGui::IsKeyDown(ImGuiKey_W)) position_delta += glm::dvec3(0.0, 0.0, -1.0);
			if (ImGui::IsKeyDown(ImGuiKey_S)) position_delta += glm::dvec3(0.0, 0.0, 1.0);
			if (ImGui::IsKeyDown(ImGuiKey_A)) position_delta += glm::dvec3(-1.0, 0.0, 0.0);
			if (ImGui::IsKeyDown(ImGuiKey_D)) position_delta += glm::dvec3(1.0, 0.0, 0.0);

			if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) distance *= 0.1;
			if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) distance *= 5.0;

			if (glm::length(position_delta) > 0.0)
			{
				target_camera = target_camera.move(position_delta * distance);
			}
		}

		if (!free_cam) target_camera.position.y = 1.5;

		return target_camera;
	}
}