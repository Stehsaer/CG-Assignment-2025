#pragma once

#include <glm/glm.hpp>
#include <gpu.hpp>
#include <graphic/util/smart-texture.hpp>

namespace app::render::target
{
	// G-buffer Object
	struct Gbuffer
	{
		// Depth Texture
		// - `D32`: 32-bit Floating Point Depth
		// - `S8`: 8-bit Stencil, for multiple light sources
		static constexpr gpu::Texture::Format depth_texture_format{
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT,
			.usage = {.depth_stencil_target = true}
		};

		// Color Target
		// - `RGB`: Actual color
		// - `A`: 0.0=None, 1.0=Fully covered
		static constexpr gpu::Texture::Format albedo_texture_format{
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
			.usage = {.sampler = true, .color_target = true}
		};

		// Lighting Info Texture
		// - `R[15:0], R[31:16]`: Octahedral-encoded Normal
		// - `G[15:8]`: Roughness
		// - `G[7:0]`: Metallic
		// - `G[23:16]`: Specular Intensity
		// - `G[31:24]`: AO
		static constexpr gpu::Texture::Format lighting_info_texture_format{
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R32G32_UINT,
			.usage = {.sampler = true, .color_target = true}
		};

		///
		/// @brief Render Pass Target Info
		///
		///
		struct Render_pass_target
		{
			std::array<SDL_GPUColorTargetInfo, 2> color_targets;
			SDL_GPUDepthStencilTargetInfo depth_stencil_target;
		};

		// Depth Texture
		graphic::Smart_texture depth_texture = {depth_texture_format};

		// Albedo Texture, at location 0
		graphic::Smart_texture albedo_texture = {albedo_texture_format};

		// Lighting Info Texture, at location 1
		graphic::Smart_texture lighting_info_texture = {lighting_info_texture_format};

		// Resize all textures
		std::expected<void, util::Error> resize(SDL_GPUDevice* device, glm::u32vec2 size) noexcept;
	};
}