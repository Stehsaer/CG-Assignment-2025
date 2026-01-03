#include "ui/capsule.hpp"

#include <imgui.h>
#include <utility>

namespace ui::capsule
{
	std::tuple<glm::vec2, glm::vec2> calc_window_pos(Position pos, glm::vec2 offset) noexcept
	{
		const auto viewport_size = ImGui::GetIO().DisplaySize;

		switch (pos)
		{
		case Position::Top_left:
			return {glm::vec2(window_margin, window_margin) + offset, glm::vec2(0.0f, 0.0f)};
		case Position::Top_right:
			return {
				glm::vec2(viewport_size.x - window_margin, window_margin) + offset,
				glm::vec2(1.0f, 0.0f)
			};
		case Position::Bottom_left:
			return {
				glm::vec2(window_margin, viewport_size.y - window_margin) + offset,
				glm::vec2(0.0f, 1.0f)
			};
		case Position::Bottom_right:
			return {
				glm::vec2(viewport_size.x - window_margin, viewport_size.y - window_margin) + offset,
				glm::vec2(1.0f, 1.0f)
			};
		case Position::Top_center:
			return {glm::vec2(viewport_size.x / 2.0f, window_margin) + offset, glm::vec2(0.5f, 0.0f)};
		case Position::Bottom_center:
			return {
				glm::vec2(viewport_size.x / 2.0f, viewport_size.y - window_margin) + offset,
				glm::vec2(0.5f, 1.0f)
			};
		case Position::Center:
			return {
				glm::vec2(viewport_size.x / 2.0f, viewport_size.y / 2.0f) + offset,
				glm::vec2(0.5f, 0.5f)
			};
		case Position::Center_left:
			return {glm::vec2(window_margin, viewport_size.y / 2.0f) + offset, glm::vec2(0.0f, 0.5f)};
		case Position::Center_right:
			return {
				glm::vec2(viewport_size.x - window_margin, viewport_size.y / 2.0f) + offset,
				glm::vec2(1.0f, 0.5f)
			};
		default:
			std::unreachable();
		}
	}

	void window(
		const char* title,
		Position position,
		const std::function<void()>& content,
		glm::ivec2 offset,
		bool less_rounding
	)
	{
		const auto [window_pos, pivot] =
			calc_window_pos(position, glm::vec2(offset) * (window_margin + window_height));
		ImGui::SetNextWindowPos({window_pos.x, window_pos.y}, ImGuiCond_Always, {pivot.x, pivot.y});
		ImGui::SetNextWindowSizeConstraints({window_height, window_height}, {FLT_MAX, FLT_MAX});
		ImGui::SetNextWindowCollapsed(false, ImGuiCond_Always);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
		ImGui::PushStyleVarX(ImGuiStyleVar_WindowPadding, window_padding);

		if (!less_rounding) ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, window_rounding);

		const auto window_opened = ImGui::Begin(
			title,
			nullptr,
			ImGuiWindowFlags_AlwaysAutoResize
				| ImGuiWindowFlags_NoMove
				| ImGuiWindowFlags_NoDecoration
				| ImGuiWindowFlags_NoBringToFrontOnFocus
		);

		if (!less_rounding) ImGui::PopStyleVar();
		ImGui::PopStyleVar(2);

		if (window_opened) content();
		ImGui::End();
	}

	bool button(const std::string& label, bool round) noexcept
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, button_size / 2.0f);
		if (!round) ImGui::PushStyleVarX(ImGuiStyleVar_FramePadding, 15.0f);

		bool result = ImGui::Button(label.c_str(), {round ? button_size : 0.0f, button_size});
		ImGui::SameLine();

		if (!round) ImGui::PopStyleVar();
		ImGui::PopStyleVar();

		return result;
	}

	void label(const std::string& text, float font_size) noexcept
	{
		ImGui::PushFont(nullptr, font_size);
		ImGui::TextUnformatted(text.c_str());
		ImGui::SameLine();
		ImGui::PopFont();
	}

	void small_label(const std::string& text) noexcept
	{
		ImGui::PushFont(nullptr, font_size * 0.6);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + font_size * 0.25f);
		ImGui::TextUnformatted(text.c_str());
		ImGui::SameLine();
		ImGui::PopFont();
	}

	void vertical_separator() noexcept
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_Border]);
		label("│", font_size);
		ImGui::PopStyleColor();
	}
}