#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <glm/glm.hpp>
#include <ranges>
#include <vector>

namespace image
{
	template <typename T>
	struct Image_container
	{
		using Pixel_type = T;

		glm::u32vec2 size;
		std::vector<T> pixels;

		///
		/// @brief Take pixel at (x, y)
		/// @note No bound check included
		/// @param self The image
		/// @param x X coordinate
		/// @param y Y coordinate
		/// @return Pixel at (x, y)
		///
		auto&& operator[](this auto&& self, size_t x, size_t y) noexcept
		{
			return self.pixels[(y * self.size.x) + x];
		}

		///
		/// @brief Map function over all pixels
		///
		/// @tparam F Function Type
		/// @param func Mapping function
		/// @return Mapped Image
		///
		template <typename F>
		auto map(this const auto& self, F func) noexcept
		{
			using Return_type = std::invoke_result_t<F, T>;
			Image_container<Return_type>
				result{.size = self.size, .pixels = std::vector<Return_type>(self.pixels.size())};

			std::ranges::copy(self.pixels | std::views::transform(func), result.pixels.begin());

			return result;
		}

		///
		/// @brief Shrink the image to half size by averaging 2x2 pixel blocks
		///
		/// @return Shrunk Image
		///
		Image_container shrink_half(this const Image_container& self) noexcept;
	};

	template <>
	Image_container<glm::vec4> Image_container<glm::vec4>::shrink_half(
		this const Image_container<glm::vec4>& self
	) noexcept;

	template <>
	Image_container<glm::u8vec4> Image_container<glm::u8vec4>::shrink_half(
		this const Image_container<glm::u8vec4>& self
	) noexcept;

	template <>
	Image_container<glm::u16vec4> Image_container<glm::u16vec4>::shrink_half(
		this const Image_container<glm::u16vec4>& self
	) noexcept;

	enum class Format
	{
		Luminance = 1,
		RG = 2,
		RGB = 3,
		RGBA = 4
	};

	enum class Precision
	{
		U8,
		U16,
		F32
	};

	namespace internal
	{
		template <Precision P>
		struct Precision_type_mapping;

		template <>
		struct Precision_type_mapping<Precision::U8>
		{
			using type = std::uint8_t;
		};

		template <>
		struct Precision_type_mapping<Precision::U16>
		{
			using type = std::uint16_t;
		};

		template <>
		struct Precision_type_mapping<Precision::F32>
		{
			using type = float;
		};
	}

	template <Precision P>
	using Precision_t = internal::Precision_type_mapping<P>::type;

	template <Precision P, Format F>
	using Pixel_t = glm::vec<static_cast<glm::length_t>(F), Precision_t<P>>;

	template <Precision P, Format F>
	using Image = Image_container<Pixel_t<P, F>>;
}