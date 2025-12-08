#include "gltf/texture.hpp"

namespace gltf
{
	std::expected<Texture, util::Error> Texture::from_tinygltf(
		const tinygltf::Model& model,
		const tinygltf::Texture& texture
	) noexcept
	{
		if (texture.source < 0) return util::Error("Texture has invalid source image index");

		if (std::cmp_greater_equal(texture.source, model.images.size()))
			return util::Error(
				std::format(
					"Texture source image index ({}) out of bounds ({})",
					texture.source,
					model.images.size()
				)
			);

		if (texture.sampler >= 0 && std::cmp_greater_equal(texture.sampler, model.samplers.size()))
			return util::Error(
				std::format(
					"Texture sampler index ({}) out of bounds ({})",
					texture.sampler,
					model.samplers.size()
				)
			);

		return Texture{
			.image_index = static_cast<uint32_t>(texture.source),
			.sampler_index = (texture.sampler >= 0)
				? std::optional<uint32_t>(static_cast<uint32_t>(texture.sampler))
				: std::nullopt,
		};
	}
}