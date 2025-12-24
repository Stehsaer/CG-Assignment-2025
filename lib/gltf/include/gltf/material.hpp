///
/// @file material.hpp
/// @brief Provides structs that stores material information, and some management classes.
///

#pragma once

#include <expected>
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <span>
#include <tiny_gltf.h>

#include "gpu/sampler.hpp"
#include "gpu/texture.hpp"
#include "image.hpp"
#include "sampler.hpp"
#include "texture.hpp"
#include "util/error.hpp"
#include "util/inline.hpp"

namespace gltf
{
	// glTF Material Alpha Mode
	enum class Alpha_mode
	{
		Opaque,
		Mask,
		Blend
	};

	// Pipeline mode
	struct Pipeline_mode
	{
		Alpha_mode alpha_mode = Alpha_mode::Opaque;
		bool double_sided = false;

		std::strong_ordering operator<=>(const Pipeline_mode&) const noexcept = default;
		bool operator==(const Pipeline_mode&) const noexcept = default;

		std::string to_string() const noexcept;
	};

	// Material Parameters
	struct Material_params
	{
		// Material factors
		struct Factor
		{
			glm::vec4 base_color_mult = glm::vec4(1.0f);
			glm::vec3 emissive_mult = glm::vec3(0.0f);
			float metallic_mult = 1.0f;
			float roughness_mult = 1.0f;
			float normal_scale = 1.0f;
			float alpha_cutoff = 1.0f;
			float occlusion_strength = 1.0f;
		};

		Factor factor = {};
		Pipeline_mode pipeline = {};
	};

	// Material object, with textures stored as indices to external texture list
	struct Material_indexed
	{
		std::optional<uint32_t> base_color, metallic_roughness, normal, occlusion, emissive;

		Material_params params;

		///
		/// @brief Create a Material from a `tinygltf::Material`
		///
		/// @param model Tinygltf model
		/// @param material Tinygltf material
		/// @return Material, or error on failure
		///
		static std::expected<Material_indexed, util::Error> from_tinygltf(
			const tinygltf::Model& model,
			const tinygltf::Material& material
		) noexcept;
	};

	// Material object, with textures stored as actual GPU texture bindings
	struct Material_gpu
	{
		SDL_GPUTextureSamplerBinding base_color, metallic_roughness, normal, occlusion, emissive;
		Material_params params;
	};

	///
	/// @brief Material cache objects, stores GPU material bindings for fast access during rendering
	/// @warning Pay extra attention to the life span of the referenced Material_cache. It should outlive all
	/// references.
	/// @note The object is forbidden to be copied or moved, to ensure the stability of references.
	///
	class Material_cache
	{
		std::vector<Material_gpu> materials;
		Material_gpu default_material;

	  public:

		///
		/// @brief A reference to a Material_cache
		/// @warning Pay extra attention to the life span of the referenced Material_cache.
		///
		struct Ref
		{
			std::span<const Material_gpu> materials;
			std::reference_wrapper<const Material_gpu> default_material;

			// Get material bind for a drawcall
			FORCE_INLINE Material_gpu operator[](std::optional<uint32_t> material_index) const noexcept
			{
				return material_index.transform([this](uint32_t index) { return materials[index]; })
					.value_or(default_material.get());
			}
		};

		Material_cache(const Material_cache&) = delete;
		Material_cache(Material_cache&&) = delete;
		Material_cache& operator=(const Material_cache&) = delete;
		Material_cache& operator=(Material_cache&&) = delete;

		Material_cache(std::vector<Material_gpu> materials, Material_gpu default_material) noexcept :
			materials(std::move(materials)),
			default_material(default_material)
		{}

		// Get reference to the material cache
		Ref ref() const noexcept;
	};

	///
	/// @brief Overall material loader and manager class
	/// @details Loads, uploads and manages materials from a glTF model. All textures are loaded into GPU
	/// memory.
	///
	class Material_list
	{
	  public:

		using Load_progress_callback = std::function<void(std::optional<size_t> current, size_t total)>;

		struct Image_config
		{
			Color_compress_mode color_mode = Color_compress_mode::RGBA8_BC7;
			Normal_compress_mode normal_mode = Normal_compress_mode::RGn_BC5;
		};

		///
		/// @brief Create `Material_list` from a glTF model
		///
		/// @param model Tinygltf model
		/// @param sampler_config Sampler creation config
		/// @param image_config Image loading config
		/// @param progress_callback Progress callback, refer to `Load_progress_callback`
		/// @return Material_list on success, or error on failure
		///
		static std::expected<Material_list, util::Error> from_tinygltf(
			SDL_GPUDevice* device,
			const tinygltf::Model& model,
			const Sampler_config& sampler_config,
			const Image_config& image_config,
			const Load_progress_callback& progress_callback = nullptr
		) noexcept;

		///
		/// @brief Generate material cache
		/// @warning Pay extra attention to the life span of the returned `Material_cache`. The material list
		/// must live longer than any material cache generated here.
		/// @return Material_cache on success, or nullopt on failure
		///
		std::optional<std::unique_ptr<Material_cache>> gen_material_cache() const noexcept;

	  private:

		struct Image_entry
		{
			std::optional<gpu::Texture> color_texture;
			std::optional<gpu::Texture> linear_texture;
			std::optional<gpu::Texture> normal_texture;
		};

		struct Image_refcount
		{
			uint32_t color_refcount = 0;   // Use count as SRGB color texture (RGB/RGBA)
			uint32_t linear_refcount = 0;  // Use count as linear texture (RGB/RGBA)
			uint32_t normal_refcount = 0;  // Use count as normal map (RG only)
		};

		static std::vector<Image_refcount> compute_image_refcounts(const tinygltf::Model& model) noexcept;

		std::vector<Image_entry> images;
		std::vector<gpu::Sampler> samplers;

		std::vector<Texture> textures;
		std::vector<Material_indexed> materials;

		std::unique_ptr<gpu::Texture> default_white;
		std::unique_ptr<gpu::Texture> default_normal;

		std::unique_ptr<gpu::Sampler> default_sampler;

		/*===== Create =====*/

		// Create default textures (fallback textures)
		std::expected<void, util::Error> create_default_textures(SDL_GPUDevice* device) noexcept;

		// Create default sampler (fallback sampler)
		std::expected<void, util::Error> create_default_sampler(SDL_GPUDevice* device) noexcept;

		// Worker thread for loading an image
		static std::expected<Image_entry, util::Error> load_image_thread(
			SDL_GPUDevice* device,
			const tinygltf::Image& image,
			const Image_config& image_config,
			Image_refcount refcount
		) noexcept;

		// Load all images from the model, concurrently
		std::expected<void, util::Error> load_images(
			SDL_GPUDevice* device,
			const tinygltf::Model& model,
			const Image_config& image_config,
			const Load_progress_callback& progress_callback
		) noexcept;

		// Load all samplers from the model
		std::expected<void, util::Error> load_samplers(
			SDL_GPUDevice* device,
			const tinygltf::Model& model,
			const Sampler_config& sampler_config
		) noexcept;

		// Load all textures from the model
		std::expected<void, util::Error> load_textures(const tinygltf::Model& model) noexcept;

		// Load all materials from the model
		std::expected<void, util::Error> load_materials(const tinygltf::Model& model) noexcept;

		/*===== Construct =====*/

		Material_list() = default;

		/*===== Acquire =====*/

		///
		/// @brief Get texture sampler binding, or default if not found
		///
		/// @param default_texture Default fallback texture
		/// @param texture_index Input texture index
		/// @return Texture sampler binding on success, or nullopt on failure
		///
		std::optional<SDL_GPUTextureSamplerBinding> get_texture_sampler_binding(
			const std::unique_ptr<gpu::Texture>& default_texture,
			std::optional<uint32_t> texture_index,
			auto element
		) const noexcept;

		///
		/// @brief Generate binding info for a material
		///
		/// @param material_index Input material index
		/// @return Material_bind on success, or nullopt on failure
		///
		std::optional<Material_gpu> gen_binding_info(std::optional<uint32_t> material_index) const noexcept;

	  public:

		Material_list(const Material_list&) = delete;
		Material_list(Material_list&&) = default;
		Material_list& operator=(const Material_list&) = delete;
		Material_list& operator=(Material_list&&) = default;
		~Material_list() = default;
	};
}
