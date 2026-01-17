#include "geometry/control-point.hpp"
#include "math.hpp"

#include <imgui.h>

namespace primitive
{
	static constexpr float HANDLE_RADIUS = 4.0f;
	static constexpr float HANDLE_BORDER_SIZE = 1.5f;
	static constexpr auto HANDLE_COLOR = IM_COL32(40, 53, 147, 255);
	static constexpr auto HANDLE_BORDER_COLOR = IM_COL32(255, 255, 255, 255);

	void ControlPoint::draw(const glm::mat4& vp_matrix) const noexcept
	{
		auto drawlist = ImGui::GetBackgroundDrawList();

		const auto& io = ImGui::GetIO();
		const auto viewport_size = glm::vec2(io.DisplaySize.x, io.DisplaySize.y);

		const auto uv = math::world_to_uv(position, vp_matrix);
		const auto viewport_pos = uv * viewport_size;

		drawlist->AddCircleFilled(
			{viewport_pos.x, viewport_pos.y},
			HANDLE_RADIUS + HANDLE_BORDER_SIZE,
			HANDLE_BORDER_COLOR
		);
		drawlist->AddCircleFilled({viewport_pos.x, viewport_pos.y}, HANDLE_RADIUS, HANDLE_COLOR);
	}

	void ControlPoint::drag(const glm::mat4& vp_matrix) noexcept
	{
		const auto& io = ImGui::GetIO();
		const auto viewport_size = glm::vec2(io.DisplaySize.x, io.DisplaySize.y);
		const auto mouse_pos = glm::vec2(io.MousePos.x, io.MousePos.y);
		const auto mouse_delta = glm::vec2(io.MouseDelta.x, io.MouseDelta.y);

		const auto uv = math::world_to_uv(position, vp_matrix);
		const auto viewport_pos = uv * viewport_size;

		if (io.WantCaptureMouse)
		{
			dragging = false;
			return;
		}

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)
			&& glm::distance(mouse_pos, viewport_pos) <= HANDLE_RADIUS + HANDLE_BORDER_SIZE)
			dragging = true;
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) dragging = false;

		if (!dragging) return;

		const auto mouse_world = math::uv_to_world(mouse_pos / viewport_size, vp_matrix);
		const auto prev_mouse_world = math::uv_to_world((mouse_pos - mouse_delta) / viewport_size, vp_matrix);
		const auto mouse_world_delta = mouse_world - prev_mouse_world;

		position += mouse_world_delta;
	}
}