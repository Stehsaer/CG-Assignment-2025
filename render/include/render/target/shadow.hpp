#pragma once

#include "graphics/util/smart-texture.hpp"
#include <SDL3/SDL_gpu.h>

namespace render::target
{
	///
	/// @brief Shadow Render Target, not tied to screen resolution
	/// @details
	/// #### Depth Texture
	/// Format: `D32@FLOAT`
	/// - `D32`: 32-bit Floating Point Depth
	///
	struct Shadow
	{
		/* Formats */

		// Depth Texture Format
		static constexpr gpu::Texture::Format depth_format{
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
			.usage = {.sampler = true, .depth_stencil_target = true}
		};

		/* Textures (CSM) */

		graphics::Auto_texture depth_texture_level0{depth_format, "Shadowmap Level 0 Texture"};
		graphics::Auto_texture depth_texture_level1{depth_format, "Shadowmap Level 1 Texture"};
		graphics::Auto_texture depth_texture_level2{depth_format, "Shadowmap Level 2 Texture"};

		/* Resize */

		std::expected<void, util::Error> resize(SDL_GPUDevice* device) noexcept;
	};
}