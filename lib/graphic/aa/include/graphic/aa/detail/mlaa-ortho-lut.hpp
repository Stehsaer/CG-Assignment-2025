#pragma once

#include <glm/glm.hpp>
#include <gpu.hpp>
#include <vector>

namespace graphic::aa
{
	///
	/// @brief Generate Ortho Area LUT
	///
	/// @param lut_size LUT Size per sub-texture
	/// @return LUT pixel data, dimensions: (size * 5) x (size * 5)
	///
	std::vector<glm::u8vec2> generate_ortho_area_lut_data(size_t lut_size) noexcept;

	///
	/// @brief Generate Ortho Area LUT Texture
	///
	/// @param lut_size LUT Size per sub-texture
	/// @return Generated LUT texture on success, or `util::Error` on failure
	///
	std::expected<gpu::Texture, util::Error> generate_ortho_area_lut(
		SDL_GPUDevice* device,
		size_t lut_size
	) noexcept;
}