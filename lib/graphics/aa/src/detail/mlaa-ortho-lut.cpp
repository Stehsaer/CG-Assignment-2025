#include "graphics/aa/detail/mlaa-ortho-lut.hpp"

#include "graphics/util/quick-create.hpp"
#include <algorithm>
#include <array>
#include <ranges>

namespace graphics::aa
{
	namespace
	{
		struct OrthoSilhouette
		{
			double left = 0, right = 0;
		};

		///
		/// @brief Pattern silhouette lookup list for orthogonal blend area LUT generation
		///
		///
		constexpr auto pattern_silhouette_list = std::to_array<OrthoSilhouette>({
			{.left = 0,    .right = 0   },
			{.left = -0.5, .right = 0   },
			{.left = 0,    .right = -0.5},
			{.left = -0.5, .right = -0.5},
			{.left = 0.5,  .right = 0   },
			{.left = 0,    .right = 0   },
			{.left = 0.5,  .right = -0.5},
			{.left = 0.5,  .right = -0.5},
			{.left = 0,    .right = 0.5 },
			{.left = -0.5, .right = 0.5 },
			{.left = 0,    .right = 0   },
			{.left = -0.5, .right = 0.5 },
			{.left = 0.5,  .right = 0.5 },
			{.left = -0.5, .right = 0.5 },
			{.left = 0.5,  .right = -0.5},
			{.left = 0,    .right = 0   }
		});

		///
		/// @brief Generate orthogonal blend area LUT block for a given pattern.
		///
		/// @param pattern Pattern index
		/// @param lut_size LUT block size
		/// @return Generated LUT block, size = (lut_size, lut_size)
		///
		std::vector<glm::u8vec2> generate_blend_lut_block(uint8_t pattern, size_t lut_size) noexcept
		{
			[[assume(pattern < 16)]];

			const auto& silhouette = pattern_silhouette_list[pattern];

			const auto get_factor_by_ratio = [&silhouette](double ratio) -> double {
				return ratio < 0.5
					? glm::mix(silhouette.left, 0.0, ratio * 2.0)
					: glm::mix(0.0, silhouette.right, (ratio - 0.5) * 2.0);
			};

			const auto compute_area = [&](uint8_t left, uint8_t right) -> glm::vec2 {
				const auto total_edge_length = left + right + 1;

				if (left == right)
				{
					const auto factor = get_factor_by_ratio(double(left) / total_edge_length);
					const auto area = glm::abs(0.25 * factor);

					if (silhouette.left * silhouette.right < 1e-6)
						return {area, area};
					else if (silhouette.left > 0)
						return {area * 2.0f, 0.0f};
					else
						return {0.0f, area * 2.0f};
				}

				const auto factor_left_edge = get_factor_by_ratio(double(left) / total_edge_length);
				const auto factor_right_edge = get_factor_by_ratio((1.0 + left) / total_edge_length);

				const auto signed_area = 0.5 * (factor_left_edge + factor_right_edge);

				return signed_area > 0 ? glm::vec2(signed_area, 0.0f) : glm::vec2(0.0f, -signed_area);
			};

			std::vector<glm::u8vec2> lut(lut_size * lut_size);

			for (const auto [right, left] : std::views::cartesian_product(
					 std::views::iota(0zu, lut_size),
					 std::views::iota(0zu, lut_size)
				 ))
			{
				const auto area = compute_area(left, right);
				const auto top_area = area.x;
				const auto bottom_area = area.y;
				lut[right * lut_size + left] = glm::u8vec2(
					glm::clamp(glm::round(bottom_area * 255.0f), 0.0f, 255.0f),
					glm::clamp(glm::round(top_area * 255.0f), 0.0f, 255.0f)
				);
			}

			return lut;
		}

		constexpr auto lut_texture_format = gpu::Texture::Format{
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R8G8_UNORM,
			.usage = {.sampler = true}
		};
	}

	std::vector<glm::u8vec2> generate_ortho_area_lut_data(size_t lut_size) noexcept
	{
		auto pattern_to_result_subtexture_idx = [](uint8_t pattern) -> std::pair<uint8_t, uint8_t> {
			const auto bit0 = pattern & 0x1;
			const auto bit1 = (pattern >> 1) & 0x1;
			const auto bit2 = (pattern >> 2) & 0x1;
			const auto bit3 = (pattern >> 3) & 0x1;

			const auto x = (bit0 << 1) | bit2;
			const auto y = (bit1 << 1) | bit3;

			return {x >= 2 ? x + 1 : x, y >= 2 ? y + 1 : y};
		};

		std::vector<glm::u8vec2> result(lut_size * lut_size * 25);
		std::ranges::fill(result, glm::u8vec2(0, 0));

		for (const auto [block_y, block_x] :
			 std::views::cartesian_product(std::views::iota(0u, 4u), std::views::iota(0u, 4u)))
		{
			const auto [result_block_x, result_block_y] =
				pattern_to_result_subtexture_idx(block_y * 4 + block_x);

			const auto block_lut = generate_blend_lut_block(block_y * 4 + block_x, lut_size);

			for (const auto [y, x] : std::views::cartesian_product(
					 std::views::iota(0zu, lut_size),
					 std::views::iota(0zu, lut_size)
				 ))
			{
				const auto result_x = result_block_x * lut_size + x;
				const auto result_y = result_block_y * lut_size + y;
				result[result_y * lut_size * 5 + result_x] = block_lut[y * lut_size + x];
			}
		}

		return result;
	}

	std::expected<gpu::Texture, util::Error> generate_ortho_area_lut(
		SDL_GPUDevice* device,
		size_t lut_size
	) noexcept
	{
		const auto image = image::ImageContainer<glm::u8vec2>{
			.size = {uint32_t(lut_size * 5), uint32_t(lut_size * 5)},
			.pixels = generate_ortho_area_lut_data(lut_size)
		};

		return graphics::create_texture_from_image(device, lut_texture_format, image, "MLAA Ortho Area LUT")
			.transform_error(util::Error::forward_fn("Create Ortho Area LUT Texture Failed"));
	}
}