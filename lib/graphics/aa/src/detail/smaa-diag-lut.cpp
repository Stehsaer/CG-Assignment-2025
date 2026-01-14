#include "graphics/aa/detail/smaa-diag-lut.hpp"
#include "graphics/util/quick-create.hpp"

#include <algorithm>
#include <array>
#include <ranges>

namespace graphics::aa
{
	namespace
	{
		constexpr size_t diag_sample_count = 1024;

		template <uint32_t N>
		std::array<glm::vec2, N> generate_hammersley() noexcept
		{
			const auto radical_inverse = [](uint32_t bits) static noexcept -> float {
				bits = (bits << 16u) | (bits >> 16u);
				bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
				bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
				bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
				bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
				return float(bits) * 2.3283064365386963e-10;
			};

			std::array<glm::vec2, N> samples{};
			for (const auto i : std::views::iota(uint32_t(0), N))
				samples[i] = glm::vec2(float(i) / float(N), radical_inverse(i));

			return samples;
		}

		const auto hammersley_samples = generate_hammersley<diag_sample_count>();

		bool point_under_line(glm::vec2 start, glm::vec2 end, glm::vec2 p) noexcept
		{
			if (glm::distance(start, end) < 0.001) return true;

			const auto mid = (start + end) * 0.5f;
			const auto a = end.y - start.y;
			const auto b = start.x - end.x;
			return dot(glm::vec2(a, b), p - mid) > 0.0f;
		}

		float pixel_diag_area(glm::vec2 start, glm::vec2 end, glm::vec2 p) noexcept
		{
			return std::ranges::count(
					   hammersley_samples | std::views::transform([&](glm::vec2 offset) -> bool {
						   return point_under_line(start, end, p + offset);
					   }),
					   true
				   )
				/ float(diag_sample_count);
		}

		glm::vec2 compute_diag_area(glm::vec2 start, glm::vec2 end, size_t left, size_t right) noexcept
		{
			const auto d = left + right + 1;
			const float a1 = pixel_diag_area(start, end + glm::vec2(d, d), glm::vec2(left + 1, left));
			const float a2 = pixel_diag_area(start, end + glm::vec2(d, d), glm::vec2(left + 1, left + 1));
			return {1 - a1, a2};
		}

		glm::vec2 compute_pattern_area(uint8_t pattern, size_t left, size_t right) noexcept
		{
			[[assume(pattern < 16)]];
			switch (pattern)
			{
			case 0:
			{
				const auto a1 = compute_diag_area(glm::vec2(1, 1), glm::vec2(1, 1), left, right);
				const auto a2 = compute_diag_area(glm::vec2(1, 0), glm::vec2(1, 0), left, right);
				return (a1 + a2) * 0.5f;
			}
			case 1:
			{
				const auto a1 = compute_diag_area(glm::vec2(1, 0), glm::vec2(0, 0), left, right);
				const auto a2 = compute_diag_area(glm::vec2(1, 0), glm::vec2(1, 0), left, right);
				return (a1 + a2) * 0.5f;
			}
			case 2:
			{
				const auto a1 = compute_diag_area(glm::vec2(0, 0), glm::vec2(1, 0), left, right);
				const auto a2 = compute_diag_area(glm::vec2(1, 0), glm::vec2(1, 0), left, right);
				return (a1 + a2) * 0.5f;
			}
			case 3:
				return compute_diag_area(glm::vec2(1, 0), glm::vec2(1, 0), left, right);
			case 4:
			{
				const auto a1 = compute_diag_area(glm::vec2(1, 1), glm::vec2(0, 0), left, right);
				const auto a2 = compute_diag_area(glm::vec2(1, 1), glm::vec2(1, 0), left, right);
				return (a1 + a2) * 0.5f;
			}
			case 5:
			{
				const auto a1 = compute_diag_area(glm::vec2(1, 1), glm::vec2(0, 0), left, right);
				const auto a2 = compute_diag_area(glm::vec2(1, 0), glm::vec2(1, 0), left, right);
				return (a1 + a2) * 0.5f;
			}
			case 6:
				return compute_diag_area(glm::vec2(1, 1), glm::vec2(1, 0), left, right);
			case 7:
			{
				const auto a1 = compute_diag_area(glm::vec2(1, 1), glm::vec2(1, 0), left, right);
				const auto a2 = compute_diag_area(glm::vec2(1, 0), glm::vec2(1, 0), left, right);
				return (a1 + a2) * 0.5f;
			}
			case 8:
			{
				const auto a1 = compute_diag_area(glm::vec2(0, 0), glm::vec2(1, 1), left, right);
				const auto a2 = compute_diag_area(glm::vec2(1, 0), glm::vec2(1, 1), left, right);
				return (a1 + a2) * 0.5f;
			}
			case 9:
				return compute_diag_area(glm::vec2(1, 0), glm::vec2(1, 1), left, right);
			case 10:
			{
				const auto a1 = compute_diag_area(glm::vec2(0, 0), glm::vec2(1, 1), left, right);
				const auto a2 = compute_diag_area(glm::vec2(1, 0), glm::vec2(1, 0), left, right);
				return (a1 + a2) * 0.5f;
			}
			case 11:
			{
				const auto a1 = compute_diag_area(glm::vec2(1, 0), glm::vec2(1, 1), left, right);
				const auto a2 = compute_diag_area(glm::vec2(1, 0), glm::vec2(1, 0), left, right);
				return (a1 + a2) * 0.5f;
			}
			case 12:
				return compute_diag_area(glm::vec2(1, 1), glm::vec2(1, 1), left, right);
			case 13:
			{
				const auto a1 = compute_diag_area(glm::vec2(1, 1), glm::vec2(1, 1), left, right);
				const auto a2 = compute_diag_area(glm::vec2(1, 0), glm::vec2(1, 1), left, right);
				return (a1 + a2) * 0.5f;
			}
			case 14:
			{
				const auto a1 = compute_diag_area(glm::vec2(1, 1), glm::vec2(1, 1), left, right);
				const auto a2 = compute_diag_area(glm::vec2(1, 1), glm::vec2(1, 0), left, right);
				return (a1 + a2) * 0.5f;
			}
			case 15:
			{
				const auto a1 = compute_diag_area(glm::vec2(1, 1), glm::vec2(1, 1), left, right);
				const auto a2 = compute_diag_area(glm::vec2(1, 0), glm::vec2(1, 0), left, right);
				return (a1 + a2) * 0.5f;
			}
			default:
				std::unreachable();
			}
		}

		std::vector<glm::u8vec2> generate_diagonal_lut_block(uint8_t pattern, size_t lut_size) noexcept
		{
			std::vector<glm::u8vec2> lut(lut_size * lut_size);

			for (const auto [right, left] : std::views::cartesian_product(
					 std::views::iota(0zu, lut_size),
					 std::views::iota(0zu, lut_size)
				 ))
			{
				const auto area = compute_pattern_area(pattern, left, right);
				const auto top_area = area.x;
				const auto bottom_area = area.y;
				lut[right * lut_size + left] = glm::u8vec2(
					glm::clamp(glm::round(top_area * 255.0f), 0.0f, 255.0f),
					glm::clamp(glm::round(bottom_area * 255.0f), 0.0f, 255.0f)
				);
			}

			return lut;
		}

		const auto diagonal_subtexture_index_list = std::to_array<glm::u8vec2>({
			{0, 0},
			{1, 0},
			{0, 2},
			{1, 2},
			{2, 0},
			{3, 0},
			{2, 2},
			{3, 2},
			{0, 1},
			{1, 1},
			{0, 3},
			{1, 3},
			{2, 1},
			{3, 1},
			{2, 3},
			{3, 3}
		});

		constexpr auto lut_texture_format = gpu::Texture::Format{
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R8G8_UNORM,
			.usage = {.sampler = true}
		};
	}

	std::vector<glm::u8vec2> generate_diagonal_area_lut_data(size_t lut_size) noexcept
	{
		std::vector<glm::u8vec2> result(lut_size * lut_size * 16);

		for (const auto pattern : std::views::iota(0u, 16u))
		{
			const auto subtexture_idx = diagonal_subtexture_index_list[pattern];
			const auto block_lut = generate_diagonal_lut_block(pattern, lut_size);

			for (const auto [y, x] : std::views::cartesian_product(
					 std::views::iota(0zu, lut_size),
					 std::views::iota(0zu, lut_size)
				 ))
			{
				const auto result_x = subtexture_idx.x * lut_size + x;
				const auto result_y = subtexture_idx.y * lut_size + y;
				result[result_y * lut_size * 4 + result_x] = block_lut[y * lut_size + x];
			}
		}

		return result;
	}

	std::expected<gpu::Texture, util::Error> generate_diagonal_area_lut(
		SDL_GPUDevice* device,
		size_t lut_size
	) noexcept
	{
		const auto image = image::ImageContainer<glm::u8vec2>{
			.size = {uint32_t(lut_size * 4), uint32_t(lut_size * 4)},
			.pixels = generate_diagonal_area_lut_data(lut_size)
		};

		return graphics::create_texture_from_image(
				   device,
				   lut_texture_format,
				   image,
				   "SMAA Diagonal Area LUT"
		)
			.transform_error(util::Error::forward_fn("Create Diagonal Area LUT Texture Failed"));
	}
}