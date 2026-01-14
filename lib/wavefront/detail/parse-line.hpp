#pragma once

#include "line-elem.hpp"

#include "util/error.hpp"

namespace wavefront::detail
{
	std::expected<ParsedLine, util::Error> parse_line(const std::string_view& slice) noexcept;

	std::expected<ParsedLine, util::Error> parse_pos(const std::vector<std::string>& parameters) noexcept;

	std::expected<ParsedLine, util::Error> parse_uv(const std::vector<std::string>& parameters) noexcept;

	std::expected<ParsedLine, util::Error> parse_normal(const std::vector<std::string>& parameters) noexcept;

	std::expected<ParsedLine, util::Error> parse_face(const std::vector<std::string>& parameters) noexcept;
}