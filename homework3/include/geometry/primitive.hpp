#pragma once

#include "control-point.hpp"
#include "vertex.hpp"

#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <vector>

namespace primitive
{
	struct Line
	{
		ControlPoint begin, end;
		glm::u8vec4 color;

		bool operator==(const Line& other) const = default;

		std::vector<LineVertex> gen_vertices(this const Line& self) noexcept;

		void draw_ui(this const Line& self, const glm::mat4& vp_matrix) noexcept;
	};

	struct Circle
	{
		ControlPoint center, border;
		glm::u8vec4 color;

		static constexpr uint32_t SEGMENTS = 64;

		bool operator==(const Circle& other) const = default;

		std::vector<LineVertex> gen_vertices(this const Circle& self) noexcept;

		void draw_ui(this const Circle& self, const glm::mat4& vp_matrix) noexcept;
	};

	struct BezierCurve
	{
		glm::u8vec4 color;
		std::vector<ControlPoint> control_points;

		bool operator==(const BezierCurve& other) const = default;

		std::vector<LineVertex> gen_vertices(this const BezierCurve& self) noexcept;

		void draw_ui(this const BezierCurve& self, const glm::mat4& vp_matrix) noexcept;

	  private:

		static constexpr uint32_t SEGMENTS = 128;
	};

	struct CubicSpline
	{
		glm::u8vec4 color;
		std::vector<ControlPoint> control_points;

		bool operator==(const CubicSpline& other) const = default;

		std::vector<LineVertex> gen_vertices(this const CubicSpline& self) noexcept;

		void draw_ui(this const CubicSpline& self, const glm::mat4& vp_matrix) noexcept;

	  private:

		static constexpr uint32_t SEGMENTS_PER_PATH = 24;

		glm::vec2 point_at(this const CubicSpline& self, float t) noexcept;
	};
}