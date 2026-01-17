#pragma once

#include "gpu/texture.hpp"
#include "graphics/util/smart-texture.hpp"

#include <SDL3/SDL_gpu.h>

namespace target
{
	struct MSAADraw
	{
		static constexpr auto MSAA_SAMPLES = SDL_GPU_SAMPLECOUNT_8;

		static constexpr auto COLOR_FORMAT = gpu::Texture::Format{
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
			.usage = {.color_target = true}
		};
		static constexpr auto DEPTH_FORMAT = gpu::Texture::Format{
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
			.usage = {.depth_stencil_target = true}
		};

		graphics::AutoTexture texture = {COLOR_FORMAT, "MSAA Texture", 1, MSAA_SAMPLES};
		graphics::AutoTexture depth_texture = {DEPTH_FORMAT, "MSAA Depth Texture", 1, MSAA_SAMPLES};

		std::expected<void, util::Error> resize(SDL_GPUDevice* device, glm::u32vec2 size) noexcept;
	};
}