#pragma once

#include <expected>
#include <glm/glm.hpp>
#include <gpu/sampler.hpp>
#include <gpu/texture.hpp>
#include <optional>
#include <tiny_gltf.h>
#include <util/error.hpp>

namespace gltf
{
	enum class Material_mode
	{
		Opaque,
		Mask,
		Blend
	};

	struct Material_params
	{
		glm::vec4 base_color_factor = glm::vec4(1.0f);
		glm::vec3 emissive_factor = glm::vec3(0.0f);
		float metallic_factor = 0.0f;
		float roughness_factor = 1.0f;
		float normal_scale = 1.0f;
		float alpha_cutoff = 1.0f;
		float occlusion_strength = 1.0f;
		bool double_sided = false;
		Material_mode alpha_mode = Material_mode::Opaque;
	};

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

	struct Material_bind
	{
		SDL_GPUTextureSamplerBinding base_color, metallic_roughness, normal, occlusion, emissive;
		Material_params params;
	};
}