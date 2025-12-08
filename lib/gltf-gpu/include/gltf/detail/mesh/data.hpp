#pragma once

#include <expected>
#include <glm/glm.hpp>
#include <optional>
#include <tiny_gltf.h>
#include <util/error.hpp>
#include <vector>

namespace gltf::detail::mesh
{
	// Get raw POSITION attribute data, indexed by original indices
	std::expected<std::vector<glm::vec3>, util::Error> get_primitive_position_raw(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive
	) noexcept;

	// Get raw NORMAL attribute data, indexed by original indices
	std::expected<std::optional<std::vector<glm::vec3>>, util::Error> get_primitive_normal_raw(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive
	) noexcept;

	// Get raw TEXCOORD_n attribute data, indexed by original indices
	std::expected<std::vector<glm::vec2>, util::Error> get_primitive_texcoord_raw(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive,
		const std::string& texcoord_name
	) noexcept;

	// Get index data, if exists
	std::expected<std::optional<std::vector<uint32_t>>, util::Error> get_primitive_index(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive
	) noexcept;

	///
	/// @brief Get POSITION attribute data
	/// @details This function does the following:
	/// 1. Acqurie the raw POSITION data from the primitive
	/// 2. If index is present, unpack the POSITION data according to the index
	/// 3. Expand to triangle list if the primitive is in triangle fan or triangle strip mode
	///
	/// @param model Tinygltf model
	/// @param primitive Tinygltf primitive
	/// @param index Optional index data
	/// @return POSITION attribute data on success, or error on failure
	///
	std::expected<std::vector<glm::vec3>, util::Error> get_position(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive,
		const std::optional<std::vector<uint32_t>>& index
	) noexcept;

	///
	/// @brief Get NORMAL attribute data
	/// @details This function does the following:
	/// 1. Acquire the raw NORMAL data from the primitive if available, otherwise generate normals from
	/// POSITION data and return
	/// 2. If index is present, unpack the NORMAL data according to the index
	/// 3. Expand to triangle list if the primitive is in triangle fan or triangle strip mode
	///
	/// @param model Tinygltf model
	/// @param primitive Tinygltf primitive
	/// @param index Optional index data
	/// @param position_vertices POSITION attribute data, used for normal generation if necessary
	/// @return NORMAL attribute data on success, or error on failure
	///
	std::expected<std::vector<glm::vec3>, util::Error> get_normal(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive,
		const std::optional<std::vector<uint32_t>>& index,
		const std::vector<glm::vec3>& position_vertices
	) noexcept;

	///
	/// @brief Get TEXCOORD attribute data
	/// @details This function does the following:
	/// 1. Acqurie the raw TEXCOORD data from the primitive
	/// 2. If index is present, unpack the TEXCOORD data according to the index
	/// 3. Expand to triangle list if the primitive is in triangle fan or triangle strip mode
	///
	/// @param model Tinygltf model
	/// @param primitive Tinygltf primitive
	/// @param index Optional index data
	/// @param texcoord_name Name of the TEXCOORD attribute (e.g. `"TEXCOORD_0"`)
	/// @return TEXCOORD attribute data on success, or error on failure
	///
	std::expected<std::vector<glm::vec2>, util::Error> get_texcoord(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive,
		const std::optional<std::vector<uint32_t>>& index,
		const std::string& texcoord_name
	) noexcept;

	///
	/// @brief Get TANGENT attribute data
	/// @details This function calculates the TANGENT data from POSITION and TEXCOORD_0 data
	///
	/// @param position_vertices POSITION attribute data
	/// @param texcoord0_vertices TEXCOORD_0 attribute data
	/// @return TANGENT attribute data
	///
	std::vector<glm::vec3> get_tangent(
		const std::vector<glm::vec3>& position_vertices,
		const std::vector<glm::vec2>& texcoord0_vertices
	) noexcept;
}