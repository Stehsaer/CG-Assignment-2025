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

	enum class Image_create_mode
	{
		Color_uncompressed,    // Raw RGBA8 Color Texture
		Color_compressed_bc3,  // BC3 Compressed Color Texture
		Color_compressed_bc7,  // BC7 Compressed Color Texture

		Normal_uncompressed,        // Uncompressed RG8/RG16 Normal Map
		Normal_all_compressed,      // BC5 Compressed Normal Map (8-bit Precision, may cause artifacts)
		Normal_16bit_uncompressed,  // Uncompressed when encounter 16-bit normal map, otherwise BC5 Compressed
	};

	///
	/// @brief Create a texture from tinygltf image
	///
	/// @param image Tinygltf image
	/// @param create_mode Create mode, see `Image_create_mode`
	/// @return Created GPU texture or error
	///
	std::expected<gpu::Texture, util::Error> create_texture_from_image(
		SDL_GPUDevice* device,
		const tinygltf::Image& image,
		Image_create_mode create_mode
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