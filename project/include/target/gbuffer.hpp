#pragma once

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include "graphics/util/smart-texture.hpp"

namespace target
{
	///
	/// @brief Gbuffer Render Target
	///
	///
	struct Gbuffer
	{
		/* Texture Formats */

		// Depth Texture Format
		static constexpr gpu::Texture::Format depth_format{
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT,
			.usage = {.sampler = true, .depth_stencil_target = true}
		};

		// Depth Value Format
		static constexpr gpu::Texture::Format depth_value_format{
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R32_FLOAT,
			.usage = {
					  .sampler = true,
					  .color_target = true,
					  .compute_storage_read = true,
					  .compute_storage_write = true
			}
		};

		// Albedo Texture Format
		static constexpr gpu::Texture::Format albedo_format{
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB,
			.usage = {.sampler = true, .color_target = true}
		};

		// Lighting Info Texture Format
		static constexpr gpu::Texture::Format lighting_info_format{
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R32G32_UINT,
			.usage = {.sampler = true, .color_target = true}
		};

		static constexpr uint32_t hiz_mip_levels = 9;

		/* Textures */

		graphics::Auto_texture depth_texture{depth_format, "Gbuffer Depth Texture"};  // Depth Texture

		graphics::Cycle_texture depth_value_texture{
			depth_value_format,
			"Gbuffer Cycled Depth Texture",
			hiz_mip_levels
		};  // Depth Value Texture

		graphics::Auto_texture albedo_texture{albedo_format, "Gbuffer Albedo Texture"};  // Albedo Texture

		graphics::Auto_texture lighting_info_texture{
			lighting_info_format,
			"Gbuffer Lighting Info Texture"
		};  // Lighting Info Texture

		/* Functions */

		// Resize all textures
		std::expected<void, util::Error> cycle(SDL_GPUDevice* device, glm::u32vec2 size) noexcept;
	};
}