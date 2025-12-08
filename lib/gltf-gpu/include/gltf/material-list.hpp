#pragma once

#include <expected>
#include <functional>
#include <glm/glm.hpp>
#include <gpu/sampler.hpp>
#include <gpu/texture.hpp>
#include <memory>
#include <optional>
#include <tiny_gltf.h>
#include <util/error.hpp>

#include "image.hpp"
#include "material.hpp"
#include "sampler.hpp"
#include "texture.hpp"

namespace gltf
{
	///
	/// @brief Overall material loader and manager
	/// @details Loads, uploads and manages materials from a glTF model. All textures are loaded into GPU
	/// memory.
	///
	class Material_list
	{
	  public:

		using Load_progress_callback = std::function<void(std::optional<size_t> current, size_t total)>;

		struct Image_config
		{
			Image_compress_mode color_mode = Image_compress_mode::RGBA8_BC7;
			Image_compress_mode normal_mode = Image_compress_mode::RGn_BC5;
		};

		static std::expected<Material_list, util::Error> create_from_model(
			SDL_GPUDevice* device,
			const tinygltf::Model& model,
			const Sampler_config& sampler_config,
			const Image_config& image_config,
			const Load_progress_callback& progress_callback = nullptr
		) noexcept;

		std::expected<Material_bind, util::Error> gen_binding_info(
			std::optional<uint32_t> material_index
		) const noexcept;

	  private:

		struct Image_entry
		{
			std::optional<gpu::Texture> color_texture;
			std::optional<gpu::Texture> normal_texture;
		};

		std::vector<Image_entry> images;
		std::vector<gpu::Sampler> samplers;

		std::vector<Texture> textures;
		std::vector<Material_indexed> materials;

		std::unique_ptr<gpu::Texture> default_base_color;
		std::unique_ptr<gpu::Texture> default_occlusion_metalness_roughness;
		std::unique_ptr<gpu::Texture> default_emissive;
		std::unique_ptr<gpu::Texture> default_normal;

		std::unique_ptr<gpu::Sampler> default_sampler;

		/* Create */

		std::expected<void, util::Error> create_default_textures(SDL_GPUDevice* device) noexcept;
		std::expected<void, util::Error> create_default_sampler(SDL_GPUDevice* device) noexcept;

		static std::expected<Image_entry, util::Error> load_image_thread(
			SDL_GPUDevice* device,
			const tinygltf::Image& image,
			const Image_config& image_config,
			Image_refcount refcount
		) noexcept;

		std::expected<void, util::Error> load_images(
			SDL_GPUDevice* device,
			const tinygltf::Model& model,
			const Image_config& image_config,
			const Load_progress_callback& progress_callback
		) noexcept;

		std::expected<void, util::Error> load_samplers(
			SDL_GPUDevice* device,
			const tinygltf::Model& model,
			const Sampler_config& sampler_config
		) noexcept;

		std::expected<void, util::Error> load_textures(const tinygltf::Model& model) noexcept;

		std::expected<void, util::Error> load_materials(const tinygltf::Model& model) noexcept;

		/* Construct */

		Material_list() = default;

		/* Acquire */

		std::expected<SDL_GPUTextureSamplerBinding, util::Error> get_texture_sampler_binding(
			const std::unique_ptr<gpu::Texture>& default_texture,
			std::optional<uint32_t> texture_index
		) const noexcept;

		std::expected<SDL_GPUTextureSamplerBinding, util::Error> get_texture_sampler_binding_normal(
			std::optional<uint32_t> texture_index
		) const noexcept;

	  public:

		Material_list(const Material_list&) = delete;
		Material_list(Material_list&&) = default;
		Material_list& operator=(const Material_list&) = delete;
		Material_list& operator=(Material_list&&) = default;
		~Material_list() = default;
	};
}