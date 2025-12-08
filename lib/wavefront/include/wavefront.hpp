#pragma once

#include <expected>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <span>
#include <string_view>
#include <util/error.hpp>
#include <vector>

namespace wavefront
{
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	struct Object
	{
		std::vector<Vertex> vertices;  // List of vertices
	};

	///
	/// @brief Parse Wavefront OBJ format model data stored in a string
	///
	/// @param content Model data content (string)
	/// @return Parsed model data, or error information on failure
	///
	std::expected<Object, util::Error> parse_string(const std::string_view& content) noexcept;

	///
	/// @brief Parse Wavefront OBJ format model data stored in binary data
	///
	/// @param content Model data content (binary)
	/// @return Parsed model data, or error information on failure
	///
	std::expected<Object, util::Error> parse_raw(std::span<const std::byte> content) noexcept;

}