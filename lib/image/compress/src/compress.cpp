#include "image/compress.hpp"

#include <bc7enc.h>
#include <mutex>
#include <ranges>
#include <rgbcx.h>
#include <stb_dxt/stb_dxt.h>

namespace image
{
	using RGBA_pixel_type = Pixel_t<Precision::U8, Format::RGBA>;
	using Block_pixel_array_8bpp = std::array<std::array<RGBA_pixel_type, 4>, 4>;

	// Extract a 4x4 block from source image
	static Block_pixel_array_8bpp extract_block(
		const ImageContainer<RGBA_pixel_type>& src,
		uint32_t block_x,
		uint32_t block_y
	) noexcept
	{
		Block_pixel_array_8bpp block_pixels;

		for (const auto [idx, row] : std::views::enumerate(block_pixels))
			std::ranges::copy(std::span(&src[block_x * 4, block_y * 4 + idx], 4), row.begin());

		return block_pixels;
	}

	// Iterate over all 4x4 blocks in the source
	template <typename Func>
		requires(std::invocable<Func, const Block_pixel_array_8bpp&, CompressionBlock&>)
	static void iterate_over_blocks(
		const ImageContainer<RGBA_pixel_type>& src,
		ImageContainer<CompressionBlock>& dst,
		Func&& compress_block_func
	) noexcept
	{
		for (const auto [output, block_coord] : std::views::zip(
				 dst.pixels,
				 std::views::cartesian_product(
					 std::views::iota(0u, src.size.y / 4),
					 std::views::iota(0u, src.size.x / 4)
				 )
			 ))
		{
			const auto [block_y, block_x] = block_coord;
			const auto block_pixels = extract_block(src, block_x, block_y);

			compress_block_func(block_pixels, output);
		}
	}

	// Generate destination image container
	static std::expected<BCImage, util::Error> generate_dst_image(
		const ImageContainer<RGBA_pixel_type>& src
	) noexcept
	{
		if (src.size.x % 4 != 0 || src.size.y % 4 != 0)
			return util::Error(
				std::format("Source image size {}x{} is not a multiple of 4x4", src.size.x, src.size.y)
			);

		if (uint64_t(src.size.x) * uint64_t(src.size.y) > (1ull << 32))
			return util::Error(std::format("Source image size {}x{} is too large", src.size.x, src.size.y));

		BCImage dst_image{
			.size = src.size,
			.pixels = std::vector<CompressionBlock>((src.size.x / 4) * (src.size.y / 4))
		};

		return dst_image;
	}

	std::expected<BCImage, util::Error> compress_to_bc3(
		const Image<Precision::U8, Format::RGBA>& src_image
	) noexcept
	{
		auto dst_image = generate_dst_image(src_image);
		if (!dst_image) return dst_image.error();

		iterate_over_blocks(
			src_image,
			*dst_image,
			[](const Block_pixel_array_8bpp& block_pixels, CompressionBlock& output) {
				stb_compress_dxt_block(
					reinterpret_cast<uint8_t*>(output.block.data()),
					reinterpret_cast<const uint8_t*>(block_pixels.data()),
					1,
					10
				);
			}
		);

		return dst_image;
	}

	std::expected<BCImage, util::Error> compress_to_bc5(
		const Image<Precision::U8, Format::RGBA>& src_image
	) noexcept
	{
		auto dst_image = generate_dst_image(src_image);
		if (!dst_image) return dst_image.error();

		iterate_over_blocks(
			src_image,
			*dst_image,
			[](const Block_pixel_array_8bpp& block_pixels, CompressionBlock& output) {
				rgbcx::encode_bc5(
					reinterpret_cast<uint8_t*>(output.block.data()),
					reinterpret_cast<const uint8_t*>(block_pixels.data())
				);
			}
		);

		return dst_image;
	}

	std::expected<BCImage, util::Error> compress_to_bc7(
		const Image<Precision::U8, Format::RGBA>& src_image
	) noexcept
	{
		static std::once_flag bc7_init_flag;

		auto dst_image = generate_dst_image(src_image);
		if (!dst_image) return dst_image.error();

		std::call_once(bc7_init_flag, [] { bc7enc_compress_block_init(); });

		bc7enc_compress_block_params params{};
		bc7enc_compress_block_params_init(&params);
		bc7enc_compress_block_params_init_perceptual_weights(&params);

		iterate_over_blocks(
			src_image,
			*dst_image,
			[&params](const Block_pixel_array_8bpp& block_pixels, CompressionBlock& output) {
				bc7enc_compress_block(
					reinterpret_cast<uint8_t*>(output.block.data()),
					reinterpret_cast<const uint8_t*>(block_pixels.data()),
					&params
				);
			}
		);

		return dst_image;
	}
}