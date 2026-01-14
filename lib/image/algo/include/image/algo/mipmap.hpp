///
/// @file mipmap.hpp
/// @brief Provides functions to generate mipmap chains for images
///

#pragma once

#include "image/repr.hpp"

namespace image
{
	///
	/// @brief Calculate number of mipmap levels for given image size and minimum size
	///
	/// @param size Image Size
	/// @param min_size Minimum Image Size
	/// @return Number of mipmap levels, including base level
	///
	size_t calc_mipmap_levels(glm::u32vec2 size, glm::u32vec2 min_size = {1, 1}) noexcept;

	///
	/// @brief Generate perceptual mipmap chain from base image
	/// @note It generates as many levels as possible if level count exceeds maximum possible value.
	///
	/// @param base_image Base Image
	/// @param levels Level count, including base level
	/// @return Generated mipmap chain
	///
	std::vector<Image<Precision::U8, Format::RGBA>> generate_perceptual_mipmap(
		const Image<Precision::U8, Format::RGBA>& base_image,
		glm::u32vec2 min_size = {1, 1}
	) noexcept;

	///
	/// @brief Generate mipmap chain from base image
	///
	/// @tparam T Pixel Type
	/// @param base_image Base Image
	/// @param levels Level count, including base level
	/// @return Generated mipmap chain
	///
	template <typename T>
	std::vector<ImageContainer<T>> generate_mipmap(
		const ImageContainer<T>& base_image,
		glm::u32vec2 min_size = {1, 1}
	) noexcept
	{
		const size_t levels = calc_mipmap_levels(base_image.size, min_size);

		std::vector<ImageContainer<T>> mipmap_chain(levels);
		mipmap_chain[0] = base_image;

		for (auto [in, out] : mipmap_chain | std::views::adjacent<2>) out = in.shrink_half();

		return mipmap_chain;
	}
}