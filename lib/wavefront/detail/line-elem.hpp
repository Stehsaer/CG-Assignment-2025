#pragma once

#include <array>
#include <compare>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <variant>

namespace wavefront::detail
{
	struct Position_line
	{
		glm::vec3 pos;
	};

	struct Uv_line
	{
		glm::vec2 uv;
	};

	struct Normal_line
	{
		glm::vec3 normal;
	};

	struct Face_line
	{
		struct Index
		{
			uint32_t pos_index, uv_index, normal_index;

			std::strong_ordering operator<=>(const Index& other) const noexcept;
		};

		Index v1, v2, v3;

		std::array<Index, 3> as_array() const noexcept;
	};

	using Parsed_line = std::variant<std::monostate, Position_line, Uv_line, Normal_line, Face_line>;
}