#include "gltf/material-list.hpp"

#include "gltf/image.hpp"
#include "gltf/sampler.hpp"

#include <future>
#include <mutex>
#include <ranges>
#include <thread_pool/thread_pool.h>

namespace gltf
{
	std::expected<void, util::Error> Material_list::create_default_textures(SDL_GPUDevice* device) noexcept
	{
		auto default_base_color_tex = gltf::create_placeholder_image(device, glm::vec4(1.0f));
		if (!default_base_color_tex) return default_base_color_tex.error();

		auto default_occlusion_metalness_roughness_tex =
			gltf::create_placeholder_image(device, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
		if (!default_occlusion_metalness_roughness_tex)
			return default_occlusion_metalness_roughness_tex.error();

		auto default_emissive_tex = gltf::create_placeholder_image(device, glm::vec4(0.0f));
		if (!default_emissive_tex) return default_emissive_tex.error();

		auto default_normal_tex = gltf::create_placeholder_image(device, glm::vec4(0.5f, 0.5f, 1.0f, 1.0f));
		if (!default_normal_tex) return default_normal_tex.error();

		default_base_color = std::make_unique<gpu::Texture>(std::move(*default_base_color_tex));
		default_occlusion_metalness_roughness =
			std::make_unique<gpu::Texture>(std::move(*default_occlusion_metalness_roughness_tex));
		default_emissive = std::make_unique<gpu::Texture>(std::move(*default_emissive_tex));
		default_normal = std::make_unique<gpu::Texture>(std::move(*default_normal_tex));

		return {};
	}

	std::expected<void, util::Error> Material_list::create_default_sampler(SDL_GPUDevice* device) noexcept
	{
		auto sampler = gpu::Sampler::create(device, {});
		if (!sampler) return sampler.error();

		default_sampler = std::make_unique<gpu::Sampler>(std::move(*sampler));
		return {};
	}

	std::expected<Material_list::Image_entry, util::Error> Material_list::load_image_thread(
		SDL_GPUDevice* device,
		const tinygltf::Image& image,
		const Image_config& image_config,
		Image_refcount refcount
	) noexcept
	{
		Image_entry entry;

		if (refcount.color_refcount > 0)
		{
			auto color_texture = gltf::create_texture_from_image(device, image, image_config.color_mode);
			if (!color_texture) return color_texture.error().propagate("Load color image at failed");

			entry.color_texture = std::move(*color_texture);
		}

		if (refcount.normal_refcount > 0)
		{
			auto normal_texture = gltf::create_texture_from_image(device, image, image_config.normal_mode);
			if (!normal_texture) return normal_texture.error().propagate("Load normal image failed");

			entry.normal_texture = std::move(*normal_texture);
		}

		return std::move(entry);
	}

	std::expected<void, util::Error> Material_list::load_images(
		SDL_GPUDevice* device,
		const tinygltf::Model& model,
		const Image_config& image_config,
		const Load_progress_callback& progress_callback
	) noexcept
	{
		if (progress_callback) progress_callback(0, model.images.size());

		const auto refcount_list = gltf::compute_image_refcounts(model);

		std::mutex progress_mutex;
		size_t progress_count = 0;

		dp::thread_pool thread_pool(std::thread::hardware_concurrency());

		auto result_futures =
			std::views::zip(model.images, refcount_list)
			| std::views::transform([&, total = refcount_list.size()](const auto& input) {
				  const auto& [image, refcount] = input;

				  return thread_pool.enqueue([&]() {
					  auto result = load_image_thread(device, image, image_config, refcount);

					  // Update progress
					  {
						  std::scoped_lock lock(progress_mutex);

						  progress_count++;
						  if (progress_callback) progress_callback(progress_count, total);
					  }

					  return result;
				  });
			  })
			| std::ranges::to<std::vector>();

		thread_pool.wait_for_tasks();

		for (auto& future : result_futures)
		{
			auto result = future.get();
			if (!result) return result.error().propagate("Load image failed");

			images.emplace_back(std::move(*result));
		}

		return {};
	}

	std::expected<void, util::Error> Material_list::load_samplers(
		SDL_GPUDevice* device,
		const tinygltf::Model& model,
		const Sampler_config& sampler_config
	) noexcept
	{
		samplers.reserve(model.samplers.size());

		for (const auto& tinygltf_sampler : model.samplers)
		{
			auto sampler = gltf::create_sampler(device, tinygltf_sampler, sampler_config);
			if (!sampler) return sampler.error();

			samplers.emplace_back(std::move(*sampler));
		}

		return {};
	}

	std::expected<void, util::Error> Material_list::load_textures(const tinygltf::Model& model) noexcept
	{
		textures.reserve(model.textures.size());

		for (const auto& [idx, tinygltf_texture] : model.textures | std::views::enumerate)
		{
			auto texture = gltf::Texture::from_tinygltf(model, tinygltf_texture);
			if (!texture)
				return texture.error().propagate(std::format("Load texture at index {} failed", idx));

			textures.emplace_back(*texture);
		}

		return {};
	}

	std::expected<void, util::Error> Material_list::load_materials(const tinygltf::Model& model) noexcept
	{
		materials.reserve(model.materials.size());

		for (const auto& [idx, tinygltf_material] : model.materials | std::views::enumerate)
		{
			auto material = Material_indexed::from_tinygltf(model, tinygltf_material);
			if (!material)
				return material.error().propagate(std::format("Load material at index {} failed", idx));

			materials.emplace_back(*material);
		}

		return {};
	}

	std::expected<SDL_GPUTextureSamplerBinding, util::Error> Material_list::get_texture_sampler_binding(
		const std::unique_ptr<gpu::Texture>& default_texture,
		std::optional<uint32_t> texture_index
	) const noexcept
	{
		if (!texture_index.has_value())
			return SDL_GPUTextureSamplerBinding{.texture = *default_texture, .sampler = *default_sampler};

		const auto& texture_entry = textures[*texture_index];

		SDL_GPUSampler* const sampler =
			texture_entry.sampler_index
				.transform([this](uint32_t sampler_index) -> SDL_GPUSampler* {
					return samplers[sampler_index];
				})
				.value_or(*default_sampler);

		const auto& color_tex = images[texture_entry.image_index].color_texture;
		if (!color_tex.has_value()) [[unlikely]]
			return util::Error(
				std::format("Texture at index {} has no color texture", texture_entry.image_index)
			);

		SDL_GPUTexture* const texture = *color_tex;

		return SDL_GPUTextureSamplerBinding{.texture = texture, .sampler = sampler};
	}

	std::expected<SDL_GPUTextureSamplerBinding, util::Error>
	Material_list::get_texture_sampler_binding_normal(std::optional<uint32_t> texture_index) const noexcept
	{
		if (!texture_index.has_value())
			return SDL_GPUTextureSamplerBinding{.texture = *default_normal, .sampler = *default_sampler};

		const auto& texture_entry = textures[*texture_index];

		SDL_GPUSampler* const sampler =
			texture_entry.sampler_index
				.transform([this](uint32_t sampler_index) -> SDL_GPUSampler* {
					return samplers[sampler_index];
				})
				.value_or(*default_sampler);

		const auto& normal_tex = images[texture_entry.image_index].normal_texture;
		if (!normal_tex.has_value()) [[unlikely]]
			return util::Error(
				std::format("Texture at index {} has no normal texture", texture_entry.image_index)
			);

		SDL_GPUTexture* const texture = *normal_tex;

		return SDL_GPUTextureSamplerBinding{.texture = texture, .sampler = sampler};
	}

	std::expected<Material_list, util::Error> Material_list::create_from_model(
		SDL_GPUDevice* device,
		const tinygltf::Model& model,
		const Sampler_config& sampler_config,
		const Image_config& image_config,
		const Load_progress_callback& progress_callback
	) noexcept
	{
		progress_callback ? progress_callback(std::nullopt, 0) : void();

		Material_list material_list;

		auto result = material_list.create_default_textures(device);
		if (!result) return result.error().propagate("Create default textures failed");

		result = material_list.create_default_sampler(device);
		if (!result) return result.error().propagate("Create default sampler failed");

		result = material_list.load_samplers(device, model, sampler_config);
		if (!result) return result.error().propagate("Load samplers failed");

		result = material_list.load_textures(model);
		if (!result) return result.error().propagate("Load textures failed");

		result = material_list.load_materials(model);
		if (!result) return result.error().propagate("Load materials failed");

		result = material_list.load_images(device, model, image_config, progress_callback);
		if (!result) return result.error().propagate("Load images failed");

		return material_list;
	}

	std::expected<Material_bind, util::Error> Material_list::gen_binding_info(
		std::optional<uint32_t> material_index
	) const noexcept
	{
		std::optional<uint32_t> base_color_index = std::nullopt;
		std::optional<uint32_t> metallic_roughness_index = std::nullopt;
		std::optional<uint32_t> normal_index = std::nullopt;
		std::optional<uint32_t> occlusion_index = std::nullopt;
		std::optional<uint32_t> emissive_index = std::nullopt;

		Material_bind bind;

		if (material_index.has_value())
		{
			if (std::cmp_greater_equal(*material_index, materials.size()))
				return util::Error(std::format("Material index {} out of bounds", *material_index));

			const auto& material = materials[*material_index];

			base_color_index = material.base_color;
			metallic_roughness_index = material.metallic_roughness;
			normal_index = material.normal;
			occlusion_index = material.occlusion;
			emissive_index = material.emissive;

			bind.params = material.params;
		}

		auto base_color_binding = get_texture_sampler_binding(default_base_color, base_color_index);
		if (!base_color_binding) return base_color_binding.error();
		bind.base_color = *base_color_binding;

		auto metallic_roughness_binding =
			get_texture_sampler_binding(default_occlusion_metalness_roughness, metallic_roughness_index);
		if (!metallic_roughness_binding) return metallic_roughness_binding.error();
		bind.metallic_roughness = *metallic_roughness_binding;

		auto normal_binding = get_texture_sampler_binding_normal(normal_index);
		if (!normal_binding) return normal_binding.error();
		bind.normal = *normal_binding;

		auto occlusion_binding =
			get_texture_sampler_binding(default_occlusion_metalness_roughness, occlusion_index);
		if (!occlusion_binding) return occlusion_binding.error();
		bind.occlusion = *occlusion_binding;

		auto emissive_binding = get_texture_sampler_binding(default_emissive, emissive_index);
		if (!emissive_binding) return emissive_binding.error();
		bind.emissive = *emissive_binding;

		return bind;
	}
}