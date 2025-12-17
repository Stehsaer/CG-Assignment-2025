#pragma once

#include "gpu/texture.hpp"
#include "graphics/util/smart-texture.hpp"

namespace target
{
	///
	/// @brief SSGI render targets
	/// @details ### Contents
	/// #### Temporal Reservoir
	/// - Texture 1: R32G32B32A32_FLOAT
	///   - R: w
	///   - G: M
	///   - B: W
	///   - A: p^(z)
	/// - Texture 2: R32G32B32A32_UINT
	///   - RGB: Ray hit position (x, y, z)
	///   - A: Ray hit normal (octahedral encoded)
	/// - Texture 3: R32G32B32A32_UINT
	///   - RGB: Ray start position (x, y, z)
	///   - A: Ray start normal (octahedral encoded)
	/// - Texture 4: R16G16B16A16_FLOAT
	///   - RGB: Radiance (r, g, b)
	///   - A: Direct light (1 for yes, 0 for no)
	///
	struct SSGI
	{
		static constexpr gpu::Texture::Format reservoir_texture1_format = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT,
			.usage = {.sampler = true, .compute_storage_read = true, .compute_storage_write = true}
		};

		static constexpr gpu::Texture::Format reservoir_texture2_format = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_UINT,
			.usage = {.sampler = true, .compute_storage_read = true, .compute_storage_write = true}
		};

		static constexpr gpu::Texture::Format reservoir_texture3_format = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_UINT,
			.usage = {.sampler = true, .compute_storage_read = true, .compute_storage_write = true}
		};

		static constexpr gpu::Texture::Format reservoir_texture4_format = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
			.usage = {.sampler = true, .compute_storage_read = true, .compute_storage_write = true}
		};

		static constexpr gpu::Texture::Format radiance_texture_format = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
			.usage = {.sampler = true, .compute_storage_read = true, .compute_storage_write = true}
		};

		graphics::Cycle_texture temporal_reservoir_texture1{
			reservoir_texture1_format,
			"SSGI Temporal Reservoir Texture 1"
		};

		graphics::Cycle_texture temporal_reservoir_texture2{
			reservoir_texture2_format,
			"SSGI Temporal Reservoir Texture 2"
		};

		graphics::Cycle_texture temporal_reservoir_texture3{
			reservoir_texture3_format,
			"SSGI Temporal Reservoir Texture 3"
		};

		graphics::Cycle_texture temporal_reservoir_texture4{
			reservoir_texture4_format,
			"SSGI Temporal Reservoir Texture 4"
		};

		graphics::Cycle_texture spatial_reservoir_texture1{
			reservoir_texture1_format,
			"SSGI Spatial Reservoir Texture 1"
		};

		graphics::Cycle_texture spatial_reservoir_texture2{
			reservoir_texture2_format,
			"SSGI Spatial Reservoir Texture 2"
		};

		graphics::Cycle_texture spatial_reservoir_texture3{
			reservoir_texture3_format,
			"SSGI Spatial Reservoir Texture 3"
		};

		graphics::Cycle_texture spatial_reservoir_texture4{
			reservoir_texture4_format,
			"SSGI Spatial Reservoir Texture 4"
		};

		graphics::Cycle_texture radiance_texture{radiance_texture_format, "SSGI Radiance Texture"};

		graphics::Auto_texture fullres_radiance_texture{
			radiance_texture_format,
			"SSGI Fullres Radiance Texture"
		};

		///
		/// @brief Resize SSGI render targets
		///
		/// @param size Swapchain size
		///
		std::expected<void, util::Error> resize(SDL_GPUDevice* device, glm::u32vec2 size) noexcept;
	};
}