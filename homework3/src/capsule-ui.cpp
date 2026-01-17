#include "capsule-ui.hpp"

#include <imgui.h>
#include <utility>

namespace capsule
{
	std::tuple<glm::vec2, glm::vec2> calc_window_pos(Position pos, glm::vec2 offset) noexcept
	{
		const auto viewport_size = ImGui::GetIO().DisplaySize;
		const auto scale = ImGui::GetStyle().FontScaleDpi;

		const auto scaled_window_margin = WINDOW_MARGIN * scale;

		switch (pos)
		{
		case Position::TopLeft:
			return {glm::vec2(scaled_window_margin, scaled_window_margin) + offset, glm::vec2(0.0f, 0.0f)};
		case Position::TopRight:
			return {
				glm::vec2(viewport_size.x - scaled_window_margin, scaled_window_margin) + offset,
				glm::vec2(1.0f, 0.0f)
			};
		case Position::BottomLeft:
			return {
				glm::vec2(scaled_window_margin, viewport_size.y - scaled_window_margin) + offset,
				glm::vec2(0.0f, 1.0f)
			};
		case Position::BottomRight:
			return {
				glm::vec2(viewport_size.x - scaled_window_margin, viewport_size.y - scaled_window_margin)
					+ offset,
				glm::vec2(1.0f, 1.0f)
			};
		case Position::TopCenter:
			return {glm::vec2(viewport_size.x / 2.0f, scaled_window_margin) + offset, glm::vec2(0.5f, 0.0f)};
		case Position::BottomCenter:
			return {
				glm::vec2(viewport_size.x / 2.0f, viewport_size.y - scaled_window_margin) + offset,
				glm::vec2(0.5f, 1.0f)
			};
		case Position::Center:
			return {
				glm::vec2(viewport_size.x / 2.0f, viewport_size.y / 2.0f) + offset,
				glm::vec2(0.5f, 0.5f)
			};
		case Position::CenterLeft:
			return {glm::vec2(scaled_window_margin, viewport_size.y / 2.0f) + offset, glm::vec2(0.0f, 0.5f)};
		case Position::CenterRight:
			return {
				glm::vec2(viewport_size.x - scaled_window_margin, viewport_size.y / 2.0f) + offset,
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
		const auto scale = ImGui::GetStyle().FontScaleDpi;

		const auto [window_pos, pivot] =
			calc_window_pos(position, glm::vec2(offset) * (WINDOW_MARGIN + WINDOW_WEIGHT) * scale);
		ImGui::SetNextWindowPos({window_pos.x, window_pos.y}, ImGuiCond_Always, {pivot.x, pivot.y});
		if (!less_rounding)
			ImGui::SetNextWindowSizeConstraints(
				{WINDOW_WEIGHT * scale, WINDOW_WEIGHT * scale},
				{FLT_MAX, FLT_MAX}
			);
		ImGui::SetNextWindowCollapsed(false, ImGuiCond_Always);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
		ImGui::PushStyleVarX(ImGuiStyleVar_WindowPadding, WINDOW_PADDING * scale);

		if (!less_rounding) ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, WINDOW_ROUNDING * scale);

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
		const auto scale = ImGui::GetStyle().FontScaleDpi;
		const auto scaled_button_size = BUTTON_SIZE * ImGui::GetStyle().FontScaleDpi;

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, scaled_button_size / 2.0f);
		if (!round) ImGui::PushStyleVarX(ImGuiStyleVar_FramePadding, WINDOW_PADDING * scale);

		const bool result =
			ImGui::Button(label.c_str(), {round ? scaled_button_size : 0.0f, scaled_button_size});
		ImGui::SameLine();

		if (!round) ImGui::PopStyleVar();
		ImGui::PopStyleVar();

		return result;
	}

	void label(const std::string& text, float font_size) noexcept
	{
		const auto scale = ImGui::GetStyle().FontScaleDpi;
		font_size *= scale;

		ImGui::PushFont(nullptr, font_size);
		ImGui::TextUnformatted(text.c_str());
		ImGui::SameLine();
		ImGui::PopFont();
	}

	void small_label(const std::string& text) noexcept
	{
		const auto scale = ImGui::GetStyle().FontScaleDpi;

		ImGui::PushFont(nullptr, FONT_SIZE * 0.6 * scale);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + FONT_SIZE * 0.25f * scale);
		ImGui::TextUnformatted(text.c_str());
		ImGui::SameLine();
		ImGui::PopFont();
	}

	void vertical_separator() noexcept
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_Border]);
		label("│", FONT_SIZE);
		ImGui::PopStyleColor();
	}
}