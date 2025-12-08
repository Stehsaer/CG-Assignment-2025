#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <tiny_gltf.h>
#include <util/error.hpp>

namespace gltf
{
	struct Texture
	{
		uint32_t image_index;
		std::optional<uint32_t> sampler_index;

		///
		/// @brief Create a `Texture` from a `tinygltf::Texture`, performing validation
		///
		/// @param model Tinygltf model
		/// @param texture Tinygltf texture
		/// @return Texture on success, or error on failure
		///
		static std::expected<Texture, util::Error> from_tinygltf(
			const tinygltf::Model& model,
			const tinygltf::Texture& texture
		) noexcept;
	};
};
