#pragma once

#include <array>
#include <compare>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <variant>

namespace wavefront::detail
{
	struct PositionLine
	{
		glm::vec3 pos;
	};

	struct UvLine
	{
		glm::vec2 uv;
	};

	struct NormalLine
	{
		glm::vec3 normal;
	};

	struct FaceLine
	{
		struct Index
		{
			uint32_t pos_index, uv_index, normal_index;

			std::strong_ordering operator<=>(const Index& other) const noexcept;
		};

		Index v1, v2, v3;

		std::array<Index, 3> as_array() const noexcept;
	};

	using ParsedLine = std::variant<std::monostate, PositionLine, UvLine, NormalLine, FaceLine>;
}