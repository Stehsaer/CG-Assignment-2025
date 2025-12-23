#include "logic/camera-control.hpp"
#include "render/param.hpp"

#include <SDL3/SDL_mouse.h>
#include <imgui.h>

namespace logic
{
	void Camera::update(const backend::SDL_context& context) noexcept
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

		const float dt = io.DeltaTime;

		lerp_camera = Flying::lerp(lerp_camera, target_camera, glm::clamp(mix_factor * dt, 0.0f, 1.0f));
	}

	render::Camera_matrices Camera::get_matrices() noexcept
	{
		const auto [width, height] = ImGui::GetIO().DisplaySize;
		const auto aspect_ratio = width / height;

		const auto view_matrix = lerp_camera.matrix();
		const auto proj_matrix = camera_projection.matrix_reverse_z(aspect_ratio);
		const auto camera_matrix = proj_matrix * view_matrix;
		const auto prev_matrix = prev_frame_camera_matrix.value_or(camera_matrix);
		prev_frame_camera_matrix = camera_matrix;

		return render::Camera_matrices{
			.view_matrix = view_matrix,
			.proj_matrix = proj_matrix,
			.prev_view_proj_matrix = prev_matrix,
			.eye_position = lerp_camera.eye_position(),
		};
	}
}