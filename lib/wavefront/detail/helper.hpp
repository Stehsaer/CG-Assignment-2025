#pragma once

#include "line-elem.hpp"

#include <charconv>
#include <optional>
#include <ranges>
#include <string_view>
#include <vector>

namespace wavefront::detail
{
	template <typename T>
	std::optional<T> to_number(const std::string_view& str) noexcept
	{
		T value;

		const auto result = std::from_chars(std::to_address(str.begin()), std::to_address(str.end()), value);

		if (result.ptr != std::to_address(str.end())
			|| result.ec == std::errc::result_out_of_range
			|| result.ec == std::errc::invalid_argument)
			return std::nullopt;

		return value;
	}

	// Extract lines of a specific type from parsed lines
	template <typename T>
	std::vector<T> extract_lines(const std::vector<ParsedLine>& lines) noexcept
	{
		return lines
			| std::views::filter([](const auto& line) { return std::holds_alternative<T>(line); })
			| std::views::transform([](const auto& line) { return std::get<T>(line); })
			| std::ranges::to<std::vector>();
	}

	std::optional<FaceLine::Index> parse_face_index(const std::string_view& slice) noexcept;
}