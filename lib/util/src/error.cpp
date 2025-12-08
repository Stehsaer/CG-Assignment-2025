#include "util/error.hpp"

#include <print>
#include <ranges>

namespace util
{
	Error::Error(std::string message, const std::source_location& location) noexcept
	{
		entries.emplace_back(location, std::move(message));
	}

	Error Error::propagate(std::string message, const std::source_location& location) const noexcept
	{
		Error new_error = *this;
		new_error.entries.emplace_back(location, std::move(message));
		return new_error;
	}

	std::function<util::Error(util::Error&&)> Error::propagate_fn(
		std::string message,
		const std::source_location& location
	) noexcept
	{
		return [msg = std::move(message), loc = location](util::Error&& err) {
			return std::move(err).propagate(msg, loc);
		};
	}

	void Error::dump_trace(std::ostream& os, bool color) const noexcept
	{
		for (const auto& [idx, entry] :
			 entries | std::views::reverse | std::views::enumerate | std::views::reverse)
		{
			const auto& [location, message] = entry;
			const auto formatted_message = message.empty() ? "" : "=> " + message;

			if (color)
				std::println(
					os,
					"[#{}] \033[93m{} \033[0m[\033[36m{}:{}\033[0m] {}",
					idx,
					location.function_name(),
					location.file_name(),
					location.line(),
					formatted_message
				);
			else
				std::println(
					os,
					"[#{}] {} [{}:{}] {}",
					idx,
					location.function_name(),
					location.file_name(),
					location.line(),
					formatted_message
				);
		}
	}
}