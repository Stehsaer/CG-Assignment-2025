#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <gpu/texture.hpp>
#include <tiny_gltf.h>

namespace gltf
{
	struct Image_refcount
	{
		uint32_t color_refcount = 0;   // Use count as color texture (RGB/RGBA)
		uint32_t normal_refcount = 0;  // Use count as normal map (RG only)
	};

	///
	/// @brief Compute reference counts for each image in the glTF model, distinguishing between color
	/// textures and normal maps.
	///
	/// @param model Tinygltf model
	/// @return List of image reference counts
	///
	std::vector<Image_refcount> compute_image_refcounts(const tinygltf::Model& model) noexcept;

	enum class Image_compress_mode
	{
		RGBA8_raw,
		RGBA8_BC3,
		RGBA8_BC7,

		RGn_raw,
		RGn_BC5,
		RG16_raw_RG8_BC5,
	};

	///
	/// @brief Create a texture from tinygltf image
	///
	/// @param image Tinygltf image
	/// @param compress_mode Compress mode, see `Image_compress_mode`
	/// @return Created GPU texture or error
	///
	std::expected<gpu::Texture, util::Error> create_texture_from_image(
		SDL_GPUDevice* device,
		const tinygltf::Image& image,
		Image_compress_mode compress_mode
	) noexcept;

	///
	/// @brief Create a placeholder image with a solid color
	///
	/// @param color Solid color, with range from 0~1
	/// @return Created GPU texture or error
	///
	std::expected<gpu::Texture, util::Error> create_placeholder_image(
		SDL_GPUDevice* device,
		glm::vec4 color
	) noexcept;
}