#pragma once

#include <span>

namespace util
{
	template <typename T>
	inline std::span<const std::byte> as_bytes(const T& value) noexcept
	{
		return std::as_bytes(std::span(value));
	}

	template <typename T>
	inline std::span<std::byte> as_writable_bytes(T& value) noexcept
	{
		return std::as_writable_bytes(std::span(value));
	}
}