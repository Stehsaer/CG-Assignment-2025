#pragma once

#include <utility>
#include <vector>

#include "gltf/mesh.hpp"

namespace gltf::detail::mesh
{
	///
	/// @brief Optimize a full primitive (with all vertex attributes)
	///
	/// @param vertices Input vertex list
	/// @return Pair of optimized vertex list and index list
	///
	std::pair<std::vector<Vertex>, std::vector<uint32_t>> optimize_primitive(
		const std::vector<Vertex>& vertices
	) noexcept;

	///
	/// @brief Optimize a position-only primitive
	///
	/// @param vertices Input vertex list (position only)
	/// @return Pair of optimized position vertex list and index list
	///
	std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> optimize_position_only_primitive(
		const std::vector<Vertex>& vertices
	) noexcept;
}