#pragma once

#include <glm/glm.hpp>
#include <optional>

namespace gltf
{
	struct Material
	{
		std::optional<uint32_t> base_color, metallic_roughness, normal, occlusion, emissive;

		glm::vec4 base_color_factor = glm::vec4(1.0f);
		glm::vec3 emissive_factor = glm::vec3(0.0f);
		float metallic_factor = 0.0f;
		float roughness_factor = 1.0f;
		float normal_scale = 1.0f;
		float alpha_cutoff = 1.0f;
		bool double_sided = false;
	};
}