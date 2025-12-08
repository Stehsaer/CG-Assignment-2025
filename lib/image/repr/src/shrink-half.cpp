#include "image/repr.hpp"

#include <glm/glm.hpp>

namespace image
{
	template <>
	Image_container<glm::vec4> Image_container<glm::vec4>::shrink_half(
		this const Image_container<glm::vec4>& self
	) noexcept
	{
		const glm::u32vec2 new_size(glm::floor(glm::vec2(self.size) / 2.0f));
		Image_container result{.size = new_size, .pixels = std::vector<glm::vec4>(new_size.x * new_size.y)};

		for (const auto [y, x] : std::views::cartesian_product(
				 std::views::iota(0u, new_size.y),
				 std::views::iota(0u, new_size.x)
			 ))
			result[x, y] =
				(self[x * 2 + 0, y * 2 + 0]
				 + self[x * 2 + 1, y * 2 + 0]
				 + self[x * 2 + 0, y * 2 + 1]
				 + self[x * 2 + 1, y * 2 + 1])
				* glm::vec4(0.25f);

		return result;
	}

	template <>
	Image_container<glm::u8vec4> Image_container<glm::u8vec4>::shrink_half(
		this const Image_container<glm::u8vec4>& self
	) noexcept
	{
		const glm::u32vec2 new_size(glm::floor(glm::vec2(self.size) / 2.0f));
		Image_container result{.size = new_size, .pixels = std::vector<glm::u8vec4>(new_size.x * new_size.y)};

		for (const auto [y, x] : std::views::cartesian_product(
				 std::views::iota(0u, new_size.y),
				 std::views::iota(0u, new_size.x)
			 ))
		{
			glm::u16vec4 sum = glm::u16vec4(self[x * 2 + 0, y * 2 + 0])
				+ glm::u16vec4(self[x * 2 + 1, y * 2 + 0])
				+ glm::u16vec4(self[x * 2 + 0, y * 2 + 1])
				+ glm::u16vec4(self[x * 2 + 1, y * 2 + 1]);
			result[x, y] = glm::u8vec4(sum / glm::u16vec4(4));
		}

		return result;
	}

	template <>
	Image_container<glm::u16vec4> Image_container<glm::u16vec4>::shrink_half(
		this const Image_container<glm::u16vec4>& self
	) noexcept
	{
		const glm::u32vec2 new_size(glm::floor(glm::vec2(self.size) / 2.0f));
		Image_container
			result{.size = new_size, .pixels = std::vector<glm::u16vec4>(new_size.x * new_size.y)};

		for (const auto [y, x] : std::views::cartesian_product(
				 std::views::iota(0u, new_size.y),
				 std::views::iota(0u, new_size.x)
			 ))
		{
			glm::u32vec4 sum = glm::u32vec4(self[x * 2 + 0, y * 2 + 0])
				+ glm::u32vec4(self[x * 2 + 1, y * 2 + 0])
				+ glm::u32vec4(self[x * 2 + 0, y * 2 + 1])
				+ glm::u32vec4(self[x * 2 + 1, y * 2 + 1]);
			result[x, y] = glm::u16vec4(sum / glm::u32vec4(4));
		}

		return result;
	}
}