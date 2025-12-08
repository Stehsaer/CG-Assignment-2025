#pragma once

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <glm/glm.hpp>
#include <image/repr.hpp>
#include <span>
#include <stb_image.h>

#include "util/error.hpp"

namespace image
{
	namespace internal
	{
		template <typename T>
		struct Load_result
		{
			T* pixels;
			int width;
			int height;
			int channels;
		};

		template <Precision P>
		Load_result<Precision_t<P>> load_from_memory(
			std::span<const std::byte> data [[maybe_unused]],
			int desired_channel [[maybe_unused]]
		) noexcept
		{
			static_assert(false, "Not implemented");
		}

		template <>
		Load_result<Precision_t<Precision::U8>> load_from_memory<Precision::U8>(
			std::span<const std::byte> data,
			int desired_channel
		) noexcept;

		template <>
		Load_result<Precision_t<Precision::U16>> load_from_memory<Precision::U16>(
			std::span<const std::byte> data,
			int desired_channel
		) noexcept;

		template <>
		Load_result<Precision_t<Precision::F32>> load_from_memory<Precision::F32>(
			std::span<const std::byte> data,
			int desired_channel
		) noexcept;

	}

	///
	/// @brief Load an image from memory
	///
	/// @tparam C Channels
	/// @tparam P Precision
	/// @param data Image data in memory
	/// @return Pixel data, or error information on failure
	///
	template <Precision P, Format F>
	std::expected<Image<P, F>, util::Error> load_from_memory(std::span<const std::byte> data) noexcept
	{
		stbi_set_flip_vertically_on_load(1);
		const auto [pixels, width, height, channels] =
			internal::load_from_memory<P>(data, static_cast<int>(F));

		if (pixels == nullptr)
			return util::Error(std::format("Load image failed: {}", stbi_failure_reason()));

		Image<P, F> image{
			.size = {width, height},
			.pixels =
				{std::from_range,
					 std::span(reinterpret_cast<const Pixel_t<P, F>*>(pixels), size_t(width) * size_t(height))}
		};

		stbi_image_free(pixels);

		return std::move(image);
	}
}