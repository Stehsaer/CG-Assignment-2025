#pragma once

#include <functional>
#include <glm/glm.hpp>
#include <string>
#include <tuple>

namespace ui::capsule
{
	constexpr float window_padding = 15.0f;
	constexpr float window_margin = 20.0f;
	constexpr float window_rounding = 30.0f;
	constexpr float window_height = window_rounding * 2;
	constexpr float font_size = 28.0f;
	constexpr float button_size = 40.0f;

	enum class Position
	{
		Top_left,
		Top_center,
		Top_right,
		Bottom_left,
		Bottom_center,
		Bottom_right,
		Center,
		Center_left,
		Center_right
	};

	///
	/// @brief Calculate window position based on predefined positions
	///
	/// @param pos Position
	/// @return (position, pivot)
	///
	std::tuple<glm::vec2, glm::vec2> calc_window_pos(Position pos, glm::vec2 offset) noexcept;

	void window(
		const char* title,
		Position position,
		const std::function<void()>& content,
		glm::ivec2 offset = {0, 0},
		bool less_rounding = false
	);

	bool button(const std::string& label, bool round = true) noexcept;

	void label(const std::string& text, float font_size = ui::capsule::font_size) noexcept;

	void small_label(const std::string& text) noexcept;

	void vertical_separator() noexcept;
}