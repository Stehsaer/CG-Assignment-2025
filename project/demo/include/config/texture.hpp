#pragma once

#include <gpu/texture.hpp>

namespace config::texture
{
	constexpr gpu::Texture::Format color_texture_format{
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.usage = {.sampler = true, .color_target = true}
	};

	constexpr gpu::Texture::Format depth_texture_format{
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT,
		.usage = {.depth_stencil_target = true}
	};

}  // namespace config::texture