#pragma once

#include "gltf/mesh.hpp"

#include <expected>
#include <vector>

namespace gltf::detail::mesh
{
	std::expected<std::vector<Vertex>, util::Error> get_primitive_list(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive
	) noexcept;
}