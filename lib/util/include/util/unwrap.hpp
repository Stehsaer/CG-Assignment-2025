#pragma once

#include "error.hpp"

namespace util
{
	struct Unwrap
	{
		std::source_location location;
		std::string message;
	};

	///
	/// @brief Error unwrapper
	/// @details Usage: `value = expected_value_or_error | util::unwrap("Error message")`
	///
	/// @param message Message for unwrapping error
	/// @param location Source location, defaults to current location
	/// @return Unwrapper object
	///
	Unwrap unwrap(
		const std::string& message = "",
		const std::source_location& location = std::source_location::current()
	) noexcept;

	template <typename T>
		requires(!std::is_same_v<T, void>)
	T operator|(std::expected<T, Error> expected, const Unwrap& unwrap)
	{
		if (!expected) throw expected.error().propagate(unwrap.message, unwrap.location);
		return std::move(expected.value());
	}

	void operator|(std::expected<void, Error> expected, const Unwrap& unwrap);
}