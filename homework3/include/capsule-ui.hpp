#pragma once

#include <functional>
#include <glm/glm.hpp>
#include <string>
#include <tuple>

namespace capsule
{
	constexpr float WINDOW_PADDING = 12.0f;
	constexpr float WINDOW_MARGIN = 12.0f;
	constexpr float WINDOW_ROUNDING = 25.0f;
	constexpr float WINDOW_WEIGHT = WINDOW_ROUNDING * 2;
	constexpr float FONT_SIZE = 22.0f;
	constexpr float BUTTON_SIZE = 34.0f;

	enum class Position
	{
		TopLeft,
		TopCenter,
		TopRight,
		BottomLeft,
		BottomCenter,
		BottomRight,
		Center,
		CenterLeft,
		CenterRight
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

	void label(const std::string& text, float font_size = FONT_SIZE) noexcept;

	void small_label(const std::string& text) noexcept;

	void vertical_separator() noexcept;
}