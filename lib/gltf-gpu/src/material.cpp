#include "gltf/material.hpp"

namespace gltf
{
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

	std::expected<Material_indexed, util::Error> Material_indexed::from_tinygltf(
		const tinygltf::Model& model,
		const tinygltf::Material& material
	) noexcept
	{
		Material_indexed mat;

		/* Root Scope */

		mat.params.alpha_cutoff = static_cast<float>(material.alphaCutoff);
		mat.params.double_sided = material.doubleSided;

		if (material.alphaMode == "OPAQUE")
			mat.params.alpha_mode = Material_mode::Opaque;
		else if (material.alphaMode == "MASK")
			mat.params.alpha_mode = Material_mode::Mask;
		else if (material.alphaMode == "BLEND")
			mat.params.alpha_mode = Material_mode::Blend;
		else
			return util::Error(std::format("Unknown alpha mode: {}", material.alphaMode));

		/* PBR */

		const auto& pbr = material.pbrMetallicRoughness;

		if (pbr.baseColorFactor.size() != 4) return util::Error("Invalid base color factor size");
		mat.params.base_color_factor = glm::vec4(
			static_cast<float>(pbr.baseColorFactor[0]),
			static_cast<float>(pbr.baseColorFactor[1]),
			static_cast<float>(pbr.baseColorFactor[2]),
			static_cast<float>(pbr.baseColorFactor[3])
		);

		mat.params.metallic_factor = static_cast<float>(pbr.metallicFactor);
		mat.params.roughness_factor = static_cast<float>(pbr.roughnessFactor);

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
		mat.params.normal_scale = static_cast<float>(normal_texture.scale);

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
		mat.params.emissive_factor = glm::vec3(
			static_cast<float>(material.emissiveFactor[0]),
			static_cast<float>(material.emissiveFactor[1]),
			static_cast<float>(material.emissiveFactor[2])
		);

		/* Occlusion */

		const auto& occlusion_texture = material.occlusionTexture;
		mat.params.occlusion_strength = static_cast<float>(occlusion_texture.strength);

		if (const auto occlusion_texture_index = get_texture_index(model, occlusion_texture);
			!occlusion_texture_index)
			return occlusion_texture_index.error();
		else
			mat.occlusion = *occlusion_texture_index;

		return mat;
	}
}