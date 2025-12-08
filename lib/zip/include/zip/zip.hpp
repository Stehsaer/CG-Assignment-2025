#pragma once

#include <expected>
#include <span>
#include <vector>

#include <util/error.hpp>

namespace zip
{
	///
	/// @brief Decompress GZIP-compressed data
	/// @note Use `Decompressor` for monad usage
	///
	/// @param data Compressed Data
	/// @param max_size Maximum acceptable size after decompression, default 1GiB
	/// @return Decompressed data, or error information on failure
	///
	std::expected<std::vector<std::byte>, util::Error> decompress(
		std::span<const std::byte> data,
		size_t max_size = 1 << 30
	) noexcept;

	///
	/// @brief Decompressor functor for monadic usage
	/// @note Example: `get_asset(...).and_then(zip::Decompressor())`
	///
	struct Decompress
	{
		size_t max_size;

		///
		/// @brief Create a Decompressor
		///
		/// @param max_size Maximum acceptable size after decompression, default 1GiB
		///
		Decompress(size_t max_size = 1 << 30);

		std::expected<std::vector<std::byte>, util::Error> operator()(
			std::span<const std::byte> data
		) const noexcept;
	};

}
