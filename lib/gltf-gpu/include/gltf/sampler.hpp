#pragma once

#include <expected>
#include <gpu/sampler.hpp>
#include <optional>
#include <tiny_gltf.h>

namespace gltf
{
	struct Sampler_config
	{
		float lod_bias = 0.0f;
		std::optional<float> anisotropy = std::nullopt;
	};

	///
	/// @brief Create a GPU sampler from a tinygltf sampler
	///
	/// @param sampler Tinygltf sampler
	/// @param config Additional sampler configuration
	/// @return GPU sampler on success, or error on failure
	///
	std::expected<gpu::Sampler, util::Error> create_sampler(
		SDL_GPUDevice* device,
		const tinygltf::Sampler& sampler,
		const Sampler_config& config
	) noexcept;
}