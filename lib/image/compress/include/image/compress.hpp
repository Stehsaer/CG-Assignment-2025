///
/// @file compress.hpp
/// @brief Provides functions to compress images into BCn formats
///

#pragma once

#include "image/repr.hpp"
#include "util/error.hpp"

namespace image
{
	///
	/// @brief BC block for 8 bits per pixel formats
	///
	///
	struct CompressionBlock
	{
		std::array<uint8_t, 16> block;
	};

	using BCImage = ImageContainer<CompressionBlock>;

	///
	/// @brief Compress a raw image into BC3 format
	///
	/// @param src_image Source image in RGBA8 format. Size must be a multiple of 4x4.
	/// @return Compressed BC3 image, or error on failure
	///
	std::expected<BCImage, util::Error> compress_to_bc3(
		const Image<Precision::U8, Format::RGBA>& src_image
	) noexcept;

	///
	/// @brief Compress a raw image into BC5 format.
	///
	/// @param src_image Source image in RGBA8 format. Size must be a multiple of 4x4. Only R and G channels
	/// are preserved and compressed
	/// @return Compressed BC5 image, or error on failure
	///
	std::expected<BCImage, util::Error> compress_to_bc5(
		const Image<Precision::U8, Format::RGBA>& src_image
	) noexcept;

	///
	/// @brief Compress a raw image into BC7 format
	///
	/// @param src_image Source image in RGBA8 format. Size must be a multiple of 4x4.
	/// @return Compressed BC7 image, or error on failure
	///
	std::expected<BCImage, util::Error> compress_to_bc7(
		const Image<Precision::U8, Format::RGBA>& src_image
	) noexcept;

	///
	/// @brief Mipmap compressing funtor
	///
	/// @details #### Example
	/// `some_image_result.and_then(image::Compress_mipmap(image::compress_to_bc3))`
	///
	/// @tparam Pixel_in Type of input pixel
	/// @tparam Pixel_out Type of output pixel
	///
	template <typename Pixel_in, typename Pixel_out>
	class CompressMipmap
	{
		std::function<std::expected<ImageContainer<Pixel_out>, util::Error>(const ImageContainer<Pixel_in>&)>
			func;

	  public:

		CompressMipmap(const CompressMipmap&) = default;
		CompressMipmap(CompressMipmap&&) = default;
		CompressMipmap& operator=(const CompressMipmap&) = default;
		CompressMipmap& operator=(CompressMipmap&&) = default;

		~CompressMipmap() = default;

		template <typename F>
			requires(!std::is_same_v<std::remove_cvref_t<F>, CompressMipmap>)
		CompressMipmap(F&& func) :
			func(std::forward<F>(func))
		{}

		std::expected<std::vector<ImageContainer<Pixel_out>>, util::Error> operator()(
			std::span<const ImageContainer<Pixel_in>> src_mipmap_chain
		) const noexcept
		{
			std::vector<ImageContainer<Pixel_out>> dst_mipmap_chain;
			dst_mipmap_chain.reserve(src_mipmap_chain.size());

			for (const auto& [idx, src_image] : src_mipmap_chain | std::views::enumerate)
			{
				auto compressed_image = func(src_image);
				if (!compressed_image)
					return compressed_image.error().forward(
						std::format("Compress mipmap level {} failed", idx)
					);

				dst_mipmap_chain.push_back(std::move(*compressed_image));
			}

			return dst_mipmap_chain;
		}
	};

	template <typename Pixel_in, typename Output>
	CompressMipmap(Output (&)(const ImageContainer<Pixel_in>&))
		-> CompressMipmap<Pixel_in, typename Output::value_type::Pixel_type>;
}