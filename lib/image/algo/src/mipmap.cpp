#include "image/algo/mipmap.hpp"
#include "image/algo/colorspace.hpp"

#include <ranges>

namespace image
{
	size_t calc_mipmap_levels(glm::u32vec2 size, glm::u32vec2 min_size) noexcept
	{
		if (size.x < min_size.x || size.y < min_size.y) return 1;

		for (size_t level = 0;; ++level)
		{
			size = glm::floor(glm::vec2(size) / 2.0f);
			if (size.x < min_size.x || size.y < min_size.y) return level + 1;
		}
	}

	std::vector<Image<Precision::U8, Format::RGBA>> generate_perceptual_mipmap(
		const Image<Precision::U8, Format::RGBA>& base_image,
		glm::u32vec2 min_size
	) noexcept
	{
		const size_t levels = calc_mipmap_levels(base_image.size, min_size);

		std::vector<ImageContainer<glm::vec4>> mipmap_chain(levels);
		mipmap_chain[0] = base_image.map([](const glm::u8vec4& pixel) {
			return colorspace::rgba_to_ycbcr_alpha(glm::vec4(pixel) / 255.0f);
		});

		for (auto [in, out] : mipmap_chain | std::views::adjacent<2>) out = in.shrink_half();

		return mipmap_chain
			| std::views::transform([](const auto& image) {
				   return image.map([](const glm::vec4& pixel) {
					   return glm::u8vec4(
						   glm::clamp(
							   colorspace::ycbcr_alpha_to_rgba(pixel) * 255.0f,
							   glm::vec4(0.0f),
							   glm::vec4(255.0f)
						   )
					   );
				   });
			   })
			| std::ranges::to<std::vector<Image<Precision::U8, Format::RGBA>>>();
	}
}