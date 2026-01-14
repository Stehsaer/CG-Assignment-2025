#pragma once

#include "util/inline.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <glm/glm.hpp>
#include <ranges>
#include <vector>

namespace image
{
	namespace detail
	{
		template <typename T>
		concept GLM_type = requires {
			typename T::length_type;
			typename T::value_type;
		};

		template <typename T>
		struct ComponentWiden;

		template <>
		struct ComponentWiden<uint8_t>
		{
			using type = uint16_t;
		};

		template <>
		struct ComponentWiden<uint16_t>
		{
			using type = uint32_t;
		};

		template <>
		struct ComponentWiden<uint32_t>
		{
			using type = uint64_t;
		};

		template <>
		struct ComponentWiden<float>
		{
			using type = float;
		};

		template <>
		struct ComponentWiden<double>
		{
			using type = double;
		};
	}

	template <typename T>
	struct ImageContainer
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
			ImageContainer<Return_type> result{
				.size = self.size,
				.pixels = std::vector<Return_type>(self.pixels.size())
			};

			std::ranges::copy(self.pixels | std::views::transform(func), result.pixels.begin());

			return result;
		}

		///
		/// @brief Shrink the image to half size by averaging 2x2 pixel blocks
		///
		/// @return Shrunk Image
		///
		ImageContainer shrink_half(this const ImageContainer& self) noexcept
			requires detail::GLM_type<T>
		{
			using Comp = typename T::value_type;
			constexpr glm::length_t len = sizeof(T) / sizeof(Comp);

			using Widened_comp = typename detail::ComponentWiden<Comp>::type;
			using Wide_t = glm::vec<len, Widened_comp>;

			const glm::u32vec2 new_size(glm::floor(glm::vec2(self.size) / 2.0f));
			ImageContainer result{.size = new_size, .pixels = std::vector<T>(new_size.x * new_size.y)};

			for (const auto [y, x] : std::views::cartesian_product(
					 std::views::iota(0u, new_size.y),
					 std::views::iota(0u, new_size.x)
				 ))
				result[x, y] =
					(Wide_t(self[x * 2 + 0, y * 2 + 0])
					 + Wide_t(self[x * 2 + 1, y * 2 + 0])
					 + Wide_t(self[x * 2 + 0, y * 2 + 1])
					 + Wide_t(self[x * 2 + 1, y * 2 + 1]))
					/ Widened_comp(4);

			return result;
		}
	};

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
		struct PrecisionTypeMapping;

		template <>
		struct PrecisionTypeMapping<Precision::U8>
		{
			using type = std::uint8_t;
		};

		template <>
		struct PrecisionTypeMapping<Precision::U16>
		{
			using type = std::uint16_t;
		};

		template <>
		struct PrecisionTypeMapping<Precision::F32>
		{
			using type = float;
		};
	}

	template <Precision P>
	using Precision_t = internal::PrecisionTypeMapping<P>::type;

	template <Precision P, Format F>
	using Pixel_t = glm::vec<static_cast<glm::length_t>(F), Precision_t<P>>;

	template <Precision P, Format F>
	using Image = ImageContainer<Pixel_t<P, F>>;
}