#pragma once

#include <glm/glm.hpp>
#include <gpu.hpp>
#include <vector>

namespace graphic::aa
{
	///
	/// @brief Generate Diagonal Area LUT
	///
	/// @param lut_size LUT Size per sub-texture
	/// @return LUT pixel data, dimensions: (size * 4, size * 4)
	///
	std::vector<glm::u8vec2> generate_diagonal_area_lut_data(size_t lut_size) noexcept;

	///
	/// @brief Generate Diagonal Area LUT Texture
	///
	/// @param lut_size LUT Size per sub-texture
	/// @return
	///
	std::expected<gpu::Texture, util::Error> generate_diagonal_area_lut(
		SDL_GPUDevice* device,
		size_t lut_size
	) noexcept;
}