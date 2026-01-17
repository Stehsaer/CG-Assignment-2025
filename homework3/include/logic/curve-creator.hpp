#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <variant>

#include "geometry/primitive.hpp"

namespace logic
{
	enum class CurveAction
	{
		None,
		AddPoint,
		PopPoint,
		Finish,
		Interrupt
	};

	CurveAction get_curve_action() noexcept;

	struct CurveContinue
	{};

	struct CurveInterrupt
	{};

	template <primitive::PrimitiveType T>
	struct CurveCreatorState;

	template <primitive::PrimitiveType T>
	struct CurveCreator : private CurveCreatorState<T>
	{
		using PrimitiveType = T;

		std::variant<CurveInterrupt, CurveContinue, T> update(
			const glm::mat4& vp_matrix,
			glm::u8vec4 color
		) noexcept;

		std::optional<T> get_curve_with_mouse(const glm::mat4& vp_matrix, glm::u8vec4 color) const noexcept;
	};

	template <>
	struct CurveCreatorState<primitive::Line>
	{
		std::optional<glm::vec2> begin;
	};

	template <>
	struct CurveCreatorState<primitive::Circle>
	{
		std::optional<glm::vec2> center;
	};

	template <>
	struct CurveCreatorState<primitive::BezierCurve>
	{
		std::vector<glm::vec2> control_points;
	};

	template <>
	struct CurveCreatorState<primitive::CubicSpline>
	{
		std::vector<glm::vec2> control_points;
	};
}
