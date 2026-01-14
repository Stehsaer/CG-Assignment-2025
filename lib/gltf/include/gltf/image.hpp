///
/// @file image.hpp
/// @brief Provides functions to create GPU textures from glTF images. Compression and mipmapping are
/// supported.
///

#pragma once

#include "gpu/texture.hpp"
#include <glm/glm.hpp>
#include <tiny_gltf.h>

namespace gltf
{
	// Compress mode for 3/4-channel color or linear textures
	enum class ColorCompressMode
	{
		RGBA8_raw,  // Load 8bit and 16bit Image as-is
		RGBA8_BC3,  // Compress to BC3 in addition to `RGBA8_raw`
		RGBA8_BC7,  // Compress to BC7 in addition to `RGBA8_raw`
	};

	// Compress mode for 2-channel normal map textures
	enum class NormalCompressMode
	{
		RGn_raw,           // Load RGn Image as-is, bit-depth will be preserved
		RGn_BC5,           // Compress to BC5 in addition to `RGn_raw`.
		RG16_raw_RG8_BC5,  // Load RG16 as-is, but compress to BC5 after loading RG8
	};

	///
	/// @brief Create a color texture from a glTF image
	/// @details The process compresses and mipmaps the image using the given config at best effort. If
	/// compression/mipmapping is not possible due to NPOT/Non-BC-Compatible dimensions, the
	/// compression/mipmapping steps will be skipped.
	///
	/// @param image Image data
	/// @param compress_mode Compression mode
	/// @param srgb Whether to use sRGB format
	/// @return Created GPU texture or error
	///
	std::expected<gpu::Texture, util::Error> create_color_texture_from_image(
		SDL_GPUDevice* device,
		const tinygltf::Image& image,
		ColorCompressMode compress_mode,
		bool srgb,
		const std::string& name
	) noexcept;

	///
	/// @brief Create a normal texture from a glTF image
	/// @details The process compresses and mipmaps the image using the given config at best effort. If
	/// compression/mipmapping is not possible due to NPOT/Non-BC-Compatible dimensions, the
	/// compression/mipmapping steps will be skipped.
	///
	/// @param image Image data
	/// @param compress_mode Compression mode
	/// @return Created GPU texture or error
	///
	std::expected<gpu::Texture, util::Error> create_normal_texture_from_image(
		SDL_GPUDevice* device,
		const tinygltf::Image& image,
		NormalCompressMode compress_mode,
		const std::string& name
	) noexcept;

	///
	/// @brief Create a placeholder image with a solid color, and dimension of 1x1
	///
	/// @param color Solid color, with range from 0~1
	/// @param name Name for the created texture
	/// @return Created GPU texture or error
	///
	std::expected<gpu::Texture, util::Error> create_placeholder_image(
		SDL_GPUDevice* device,
		glm::vec4 color,
		const std::string& name
	) noexcept;
}