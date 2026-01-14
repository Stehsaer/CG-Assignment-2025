#pragma once

#include "line-elem.hpp"
#include "wavefront.hpp"

#include <expected>
#include <string_view>
#include <vector>

namespace wavefront::detail
{
	std::expected<std::vector<ParsedLine>, util::Error> parse_tokenize(
		const std::string_view& content
	) noexcept;

	std::expected<Object, util::Error> build_object(const std::vector<ParsedLine>& lines) noexcept;
}