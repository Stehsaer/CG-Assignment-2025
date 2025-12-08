#pragma once

#include "error.hpp"

#include <expected>
#include <map>
#include <span>
#include <string>

namespace util
{
	std::expected<std::span<const std::byte>, util::Error> get_asset(
		const std::map<std::string, std::span<const std::byte>>& data,
		const std::string& name
	) noexcept;
}