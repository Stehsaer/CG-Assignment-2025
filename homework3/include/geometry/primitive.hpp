#pragma once

#include "vertex.hpp"

#include <glm/glm.hpp>
#include <vector>

namespace primitive
{
	struct Line
	{
		LineVertex begin, end;

		bool operator==(const Line& other) const = default;

		std::vector<LineVertex> gen_vertices(this const Line& self) noexcept;
	};

	struct Circle
	{
		glm::vec2 center;
		glm::u8vec4 color;
		float radius;

		static constexpr uint32_t SEGMENTS = 64;

		static Circle from_three_points(
			const glm::vec2& p1,
			const glm::vec2& p2,
			const glm::vec2& p3,
			const glm::u8vec4& color
		) noexcept;

		bool operator==(const Circle& other) const = default;

		std::vector<LineVertex> gen_vertices(this const Circle& self) noexcept;
	};

	struct BezierCurve
	{
		glm::u8vec4 color;
		std::vector<glm::vec2> control_points;

		static constexpr uint32_t SEGMENTS = 128;

		bool operator==(const BezierCurve& other) const = default;

		std::vector<LineVertex> gen_vertices(this const BezierCurve& self) noexcept;
	};

	struct CubicSpline
	{
		glm::u8vec4 color;
		std::vector<glm::vec2> control_points;

		static constexpr uint32_t SEGMENTS_PER_PATH = 24;

		bool operator==(const CubicSpline& other) const = default;

		std::vector<LineVertex> gen_vertices(this const CubicSpline& self) noexcept;
	};
}