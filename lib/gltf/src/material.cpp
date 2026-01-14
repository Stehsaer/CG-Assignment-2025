#include "gltf/material.hpp"
#include "gltf/image.hpp"

#include <format>
#include <ranges>
#include <thread_pool/thread_pool.h>

namespace gltf
{
	std::string PipelineMode::to_string() const noexcept
	{
		std::string alpha_mode_str;

		switch (alpha_mode)
		{
		case AlphaMode::Opaque:
			alpha_mode_str = "Opaque";
			break;
		case AlphaMode::Mask:
			alpha_mode_str = "Mask";
			break;
		case AlphaMode::Blend:
			alpha_mode_str = "Blend";
			break;
		}

		if (double_sided)
			return std::format("Pipeline_mode(Alpha_mode: {}, Double Sided)", alpha_mode_str);
		else
			return std::format("Pipeline_mode(Alpha_mode: {}, Single Sided)", alpha_mode_str);
	}

	static std::expected<std::optional<uint32_t>, util::Error> get_texture_index(
		const tinygltf::Model& model,
		const auto& texture_info
	) noexcept
	{
		if (std::cmp_greater_equal(texture_info.index, model.textures.size()))
			return util::Error("Texture index out of bounds");

		if (texture_info.index < 0)
			return std::nullopt;
		else
			return static_cast<uint32_t>(texture_info.index);
	}

	MaterialCache::Ref MaterialCache::ref() const noexcept
	{
		return {.materials = materials, .default_material = std::ref(default_material)};
	}

	std::expected<MaterialIndexed, util::Error> MaterialIndexed::from_tinygltf(
		const tinygltf::Model& model,
		const tinygltf::Material& material
	) noexcept
	{
		MaterialIndexed mat;

		/* Root Scope */

		mat.params.factor.alpha_cutoff = static_cast<float>(material.alphaCutoff);
		mat.params.pipeline.double_sided = material.doubleSided;

		if (material.alphaMode == "OPAQUE")
			mat.params.pipeline.alpha_mode = AlphaMode::Opaque;
		else if (material.alphaMode == "MASK")
			mat.params.pipeline.alpha_mode = AlphaMode::Mask;
		else if (material.alphaMode == "BLEND")
			mat.params.pipeline.alpha_mode = AlphaMode::Blend;
		else
			return util::Error(std::format("Unknown alpha mode: {}", material.alphaMode));

		/* PBR */

		const auto& pbr = material.pbrMetallicRoughness;

		if (pbr.baseColorFactor.size() != 4) return util::Error("Invalid base color factor size");
		mat.params.factor.base_color_mult = glm::vec4(
			static_cast<float>(pbr.baseColorFactor[0]),
			static_cast<float>(pbr.baseColorFactor[1]),
			static_cast<float>(pbr.baseColorFactor[2]),
			static_cast<float>(pbr.baseColorFactor[3])
		);

		mat.params.factor.metallic_mult = static_cast<float>(pbr.metallicFactor);
		mat.params.factor.roughness_mult = static_cast<float>(pbr.roughnessFactor);

		// Base Color
		if (const auto base_color_texture_index = get_texture_index(model, pbr.baseColorTexture);
			!base_color_texture_index)
			return base_color_texture_index.error();
		else
			mat.base_color = *base_color_texture_index;

		// Metallic Roughness
		if (const auto metallic_roughness_texture_index =
				get_texture_index(model, pbr.metallicRoughnessTexture);
			!metallic_roughness_texture_index)
			return metallic_roughness_texture_index.error();
		else
			mat.metallic_roughness = *metallic_roughness_texture_index;

		/* Normal */

		const auto& normal_texture = material.normalTexture;
		mat.params.factor.normal_scale = static_cast<float>(normal_texture.scale);

		if (const auto normal_texture_index = get_texture_index(model, normal_texture); !normal_texture_index)
			return normal_texture_index.error();
		else
			mat.normal = *normal_texture_index;

		/* Emissive */

		if (const auto emissive_texture_index = get_texture_index(model, material.emissiveTexture);
			!emissive_texture_index)
			return emissive_texture_index.error();
		else
			mat.emissive = *emissive_texture_index;

		if (material.emissiveFactor.size() != 3) return util::Error("Invalid emissive factor size");
		mat.params.factor.emissive_mult = glm::vec3(
			static_cast<float>(material.emissiveFactor[0]),
			static_cast<float>(material.emissiveFactor[1]),
			static_cast<float>(material.emissiveFactor[2])
		);

		if (const auto found = material.extensions.find("KHR_materials_emissive_strength");
			found != material.extensions.end())
		{
			const auto& ext = found->second;
			if (ext.Has("emissiveStrength") && ext.Get("emissiveStrength").IsNumber())
			{
				mat.params.factor.emissive_mult *=
					static_cast<float>(ext.Get("emissiveStrength").Get<double>());
			}
		}

		/* Occlusion */

		const auto& occlusion_texture = material.occlusionTexture;
		mat.params.factor.occlusion_strength = static_cast<float>(occlusion_texture.strength);

		if (const auto occlusion_texture_index = get_texture_index(model, occlusion_texture);
			!occlusion_texture_index)
			return occlusion_texture_index.error();
		else
			mat.occlusion = *occlusion_texture_index;

		return mat;
	}

	std::vector<MaterialList::ImageRefCount> MaterialList::compute_image_refcounts(
		const tinygltf::Model& model
	) noexcept
	{
		std::vector<ImageRefCount> refcount_list(model.images.size());

		const auto count_texture = [&model, &refcount_list](auto proj, const auto& texture_info) {
			if (const auto index = texture_info.index;
				index >= 0 && std::cmp_less(index, model.textures.size()))
			{
				if (const auto source = model.textures[index].source;
					source >= 0 && std::cmp_less(source, model.images.size()))
				{
					std::invoke(proj, refcount_list[source])++;
				}
			}
		};

		for (const auto& material : model.materials)
		{
			count_texture(&ImageRefCount::color_refcount, material.pbrMetallicRoughness.baseColorTexture);
			count_texture(
				&ImageRefCount::linear_refcount,
				material.pbrMetallicRoughness.metallicRoughnessTexture
			);
			count_texture(&ImageRefCount::linear_refcount, material.occlusionTexture);
			count_texture(&ImageRefCount::color_refcount, material.emissiveTexture);
			count_texture(&ImageRefCount::normal_refcount, material.normalTexture);
		}

		return refcount_list;
	}

	std::optional<std::unique_ptr<MaterialCache>> MaterialList::gen_material_cache() const noexcept
	{
		std::vector<MaterialGPU> material_binds;
		material_binds.reserve(materials.size());

		for (const auto idx : std::views::iota(0zu, materials.size()))
		{
			auto bind = gen_binding_info(idx);
			if (!bind.has_value()) return std::nullopt;
			material_binds.push_back(*bind);
		}

		const auto default_bind = gen_binding_info(std::nullopt);
		if (!default_bind.has_value()) return std::nullopt;

		return std::make_unique<MaterialCache>(std::move(material_binds), *default_bind);
	}

	std::expected<void, util::Error> MaterialList::create_default_textures(SDL_GPUDevice* device) noexcept
	{
		auto default_white_tex =
			gltf::create_placeholder_image(device, glm::vec4(1.0f), "GLTF Default Base Color");
		if (!default_white_tex) return default_white_tex.error();

		auto default_normal_tex =
			gltf::create_placeholder_image(device, glm::vec4(0.5f, 0.5f, 1.0f, 1.0f), "GLTF Default Normal");
		if (!default_normal_tex) return default_normal_tex.error();

		default_white = std::make_unique<gpu::Texture>(std::move(*default_white_tex));
		default_normal = std::make_unique<gpu::Texture>(std::move(*default_normal_tex));

		return {};
	}

	std::expected<void, util::Error> MaterialList::create_default_sampler(SDL_GPUDevice* device) noexcept
	{
		auto sampler = gpu::Sampler::create(device, {});
		if (!sampler) return sampler.error();

		default_sampler = std::make_unique<gpu::Sampler>(std::move(*sampler));
		return {};
	}

	std::expected<MaterialList::ImageEntry, util::Error> MaterialList::load_image_thread(
		SDL_GPUDevice* device,
		const tinygltf::Image& image,
		const ImageConfig& image_config,
		ImageRefCount refcount
	) noexcept
	{
		ImageEntry entry;

		if (refcount.color_refcount > 0)
		{
			auto color_texture = gltf::create_color_texture_from_image(
				device,
				image,
				image_config.color_mode,
				true,
				std::format("GLTF Image '{}'", image.name)
			);
			if (!color_texture) return color_texture.error().forward("Load color image failed");

			entry.color_texture = std::move(*color_texture);
		}

		if (refcount.linear_refcount > 0)
		{
			auto linear_texture = gltf::create_color_texture_from_image(
				device,
				image,
				image_config.color_mode,
				false,
				std::format("GLTF Image '{}'", image.name)
			);
			if (!linear_texture) return linear_texture.error().forward("Load linear image failed");

			entry.linear_texture = std::move(*linear_texture);
		}

		if (refcount.normal_refcount > 0)
		{
			auto normal_texture = gltf::create_normal_texture_from_image(
				device,
				image,
				image_config.normal_mode,
				std::format("GLTF Image '{}'", image.name)
			);
			if (!normal_texture) return normal_texture.error().forward("Load normal image failed");

			entry.normal_texture = std::move(*normal_texture);
		}

		return entry;
	}

	std::expected<void, util::Error> MaterialList::load_images(
		SDL_GPUDevice* device,
		const tinygltf::Model& model,
		const ImageConfig& image_config,
		const Load_progress_callback& progress_callback
	) noexcept
	{
		if (progress_callback) progress_callback(0, model.images.size());

		const auto refcount_list = compute_image_refcounts(model);

		auto progress_mutex = std::make_shared<std::mutex>();
		auto progress_count = std::make_shared<std::atomic<size_t>>(0);

		dp::thread_pool thread_pool(std::thread::hardware_concurrency());

		auto result_futures =
			std::views::zip(model.images, refcount_list)
			| std::views::transform([&, total = refcount_list.size()](const auto& input) {
				  const auto& [image, refcount] = input;

				  return thread_pool.enqueue(
					  [device,
					   image_config,
					   refcount,
					   progress_mutex,
					   progress_count,
					   progress_callback,
					   total,
					   image]() {
						  auto result = load_image_thread(device, image, image_config, refcount);

						  // Update progress
						  {
							  std::scoped_lock lock(*progress_mutex);

							  (*progress_count)++;
							  if (progress_callback) progress_callback((*progress_count).load(), total);
						  }

						  return result;
					  }
				  );
			  })
			| std::ranges::to<std::vector>();

		thread_pool.wait_for_tasks();

		for (auto& future : result_futures)
		{
			auto result = future.get();
			if (!result) return result.error().forward("Load image failed");

			images.emplace_back(std::move(*result));
		}

		return {};
	}

	std::expected<void, util::Error> MaterialList::load_samplers(
		SDL_GPUDevice* device,
		const tinygltf::Model& model,
		const SamplerConfig& sampler_config
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

	std::expected<void, util::Error> MaterialList::load_textures(const tinygltf::Model& model) noexcept
	{
		textures.reserve(model.textures.size());

		for (const auto& [idx, tinygltf_texture] : model.textures | std::views::enumerate)
		{
			auto texture = gltf::Texture::from_tinygltf(model, tinygltf_texture);
			if (!texture) return texture.error().forward(std::format("Load texture at index {} failed", idx));

			textures.emplace_back(*texture);
		}

		return {};
	}

	std::expected<void, util::Error> MaterialList::load_materials(const tinygltf::Model& model) noexcept
	{
		materials.reserve(model.materials.size());

		for (const auto& [idx, tinygltf_material] : model.materials | std::views::enumerate)
		{
			auto material = MaterialIndexed::from_tinygltf(model, tinygltf_material);
			if (!material)
				return material.error().forward(std::format("Load material at index {} failed", idx));

			materials.emplace_back(*material);
		}

		return {};
	}

	std::optional<SDL_GPUTextureSamplerBinding> MaterialList::get_texture_sampler_binding(
		const std::unique_ptr<gpu::Texture>& default_texture,
		std::optional<uint32_t> texture_index,
		auto element
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

		const auto& color_tex = std::invoke(element, images[texture_entry.image_index]);
		if (!color_tex.has_value()) [[unlikely]]
			return std::nullopt;

		SDL_GPUTexture* const texture = *color_tex;

		return SDL_GPUTextureSamplerBinding{.texture = texture, .sampler = sampler};
	}

	std::expected<MaterialList, util::Error> MaterialList::from_tinygltf(
		SDL_GPUDevice* device,
		const tinygltf::Model& model,
		const SamplerConfig& sampler_config,
		const ImageConfig& image_config,
		const Load_progress_callback& progress_callback
	) noexcept
	{
		if (progress_callback) progress_callback(std::nullopt, 0);

		MaterialList material_list;

		auto result = material_list.create_default_textures(device);
		if (!result) return result.error().forward("Create default textures failed");

		result = material_list.create_default_sampler(device);
		if (!result) return result.error().forward("Create default sampler failed");

		result = material_list.load_samplers(device, model, sampler_config);
		if (!result) return result.error().forward("Load samplers failed");

		result = material_list.load_textures(model);
		if (!result) return result.error().forward("Load textures failed");

		result = material_list.load_materials(model);
		if (!result) return result.error().forward("Load materials failed");

		result = material_list.load_images(device, model, image_config, progress_callback);
		if (!result) return result.error().forward("Load images failed");

		return material_list;
	}

	std::optional<MaterialGPU> MaterialList::gen_binding_info(
		std::optional<uint32_t> material_index
	) const noexcept
	{
		std::optional<uint32_t> base_color_index = std::nullopt;
		std::optional<uint32_t> metallic_roughness_index = std::nullopt;
		std::optional<uint32_t> normal_index = std::nullopt;
		std::optional<uint32_t> occlusion_index = std::nullopt;
		std::optional<uint32_t> emissive_index = std::nullopt;

		MaterialGPU bind;

		if (material_index.has_value()) [[likely]]
		{
			if (std::cmp_greater_equal(*material_index, materials.size())) return std::nullopt;

			const auto& material = materials[*material_index];

			base_color_index = material.base_color;
			metallic_roughness_index = material.metallic_roughness;
			normal_index = material.normal;
			occlusion_index = material.occlusion;
			emissive_index = material.emissive;

			bind.params = material.params;
		}

		auto base_color_binding =
			get_texture_sampler_binding(default_white, base_color_index, &ImageEntry::color_texture);
		if (!base_color_binding) return std::nullopt;
		bind.base_color = *base_color_binding;

		auto metallic_roughness_binding =
			get_texture_sampler_binding(default_white, metallic_roughness_index, &ImageEntry::linear_texture);
		if (!metallic_roughness_binding) return std::nullopt;
		bind.metallic_roughness = *metallic_roughness_binding;

		auto normal_binding =
			get_texture_sampler_binding(default_normal, normal_index, &ImageEntry::normal_texture);
		if (!normal_binding) return std::nullopt;
		bind.normal = *normal_binding;

		auto occlusion_binding =
			get_texture_sampler_binding(default_white, occlusion_index, &ImageEntry::linear_texture);
		if (!occlusion_binding) return std::nullopt;
		bind.occlusion = *occlusion_binding;

		auto emissive_binding =
			get_texture_sampler_binding(default_white, emissive_index, &ImageEntry::color_texture);
		if (!emissive_binding) return std::nullopt;
		bind.emissive = *emissive_binding;

		return bind;
	}
}