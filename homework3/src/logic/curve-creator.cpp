#include "logic/curve-creator.hpp"
#include "geometry/primitive.hpp"
#include "math.hpp"

#include <imgui.h>
#include <ranges>

namespace logic
{
	static glm::vec2 mouse_uv() noexcept
	{
		const auto& io = ImGui::GetIO();
		return {io.MousePos.x / io.DisplaySize.x, io.MousePos.y / io.DisplaySize.y};
	}

	CurveAction get_curve_action() noexcept
	{
		auto& io = ImGui::GetIO();

		if (io.WantCaptureMouse || io.WantCaptureKeyboard) return CurveAction::None;

		if (ImGui::IsKeyPressed(ImGuiKey_Escape)) return CurveAction::Interrupt;
		if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
			return CurveAction::Finish;
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return CurveAction::AddPoint;
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) return CurveAction::PopPoint;

		return CurveAction::None;
	}

	template <>
	std::optional<primitive::Line> CurveCreator<primitive::Line>::get_curve_with_mouse(
		const glm::mat4& vp_matrix,
		glm::u8vec4 color
	) const noexcept
	{
		if (!begin.has_value()) return std::nullopt;

		const auto mouse_world_pos = math::uv_to_world(mouse_uv(), vp_matrix);
		return primitive::Line{
			.begin = {.position = *begin},
			.end = {.position = mouse_world_pos},
			.color = color
		};
	}

	template <>
	std::optional<primitive::Circle> CurveCreator<primitive::Circle>::get_curve_with_mouse(
		const glm::mat4& vp_matrix,
		glm::u8vec4 color
	) const noexcept
	{
		if (!center.has_value()) return std::nullopt;

		const auto mouse_world_pos = math::uv_to_world(mouse_uv(), vp_matrix);
		return primitive::Circle{
			.center = {.position = *center},
			.border = {.position = mouse_world_pos},
			.color = color
		};
	}

	template <>
	std::optional<primitive::BezierCurve> CurveCreator<primitive::BezierCurve>::get_curve_with_mouse(
		const glm::mat4& vp_matrix,
		glm::u8vec4 color
	) const noexcept
	{
		auto control_points = this->control_points;

		const auto mouse_world_pos = math::uv_to_world(mouse_uv(), vp_matrix);
		control_points.push_back(mouse_world_pos);

		return primitive::BezierCurve{
			.color = color,
			.control_points =
				std::views::transform(
					control_points,
					[](const glm::vec2& pos) { return primitive::ControlPoint{.position = pos}; }
				)
				| std::ranges::to<std::vector>()
		};
	}

	template <>
	std::optional<primitive::CubicSpline> CurveCreator<primitive::CubicSpline>::get_curve_with_mouse(
		const glm::mat4& vp_matrix,
		glm::u8vec4 color
	) const noexcept
	{
		auto control_points = this->control_points;

		const auto mouse_world_pos = math::uv_to_world(mouse_uv(), vp_matrix);
		control_points.push_back(mouse_world_pos);

		return primitive::CubicSpline{
			.color = color,
			.control_points =
				std::views::transform(
					control_points,
					[](const glm::vec2& pos) { return primitive::ControlPoint{.position = pos}; }
				)
				| std::ranges::to<std::vector>()
		};
	}

	template <>
	std::variant<CurveInterrupt, CurveContinue, primitive::Line> CurveCreator<primitive::Line>::update(
		const glm::mat4& vp_matrix,
		glm::u8vec4 color
	) noexcept
	{
		const auto mouse_world_pos = math::uv_to_world(mouse_uv(), vp_matrix);

		switch (get_curve_action())
		{
		case CurveAction::None:
			return CurveContinue();

		case CurveAction::AddPoint:
			if (begin.has_value())
				return get_curve_with_mouse(vp_matrix, color).value();
			else
			{
				begin = mouse_world_pos;
				return CurveContinue();
			}

		case CurveAction::PopPoint:
			begin.reset();
			return CurveContinue();

		case CurveAction::Finish:
			return get_curve_with_mouse(vp_matrix, color).value();
		case CurveAction::Interrupt:
			return CurveInterrupt();
		}

		return CurveInterrupt();
	}

	template <>
	std::variant<CurveInterrupt, CurveContinue, primitive::Circle> CurveCreator<primitive::Circle>::update(
		const glm::mat4& vp_matrix,
		glm::u8vec4 color
	) noexcept
	{
		const auto mouse_world_pos = math::uv_to_world(mouse_uv(), vp_matrix);

		switch (get_curve_action())
		{
		case CurveAction::None:
			return CurveContinue();

		case CurveAction::AddPoint:
			if (center.has_value())
				return get_curve_with_mouse(vp_matrix, color).value();
			else
			{
				center = mouse_world_pos;
				return CurveContinue();
			}

		case CurveAction::PopPoint:
			center.reset();
			return CurveContinue();

		case CurveAction::Finish:
			return get_curve_with_mouse(vp_matrix, color).value();

		case CurveAction::Interrupt:
			return CurveInterrupt();
		}

		return CurveInterrupt();
	}

	template <>
	std::variant<CurveInterrupt, CurveContinue, primitive::BezierCurve> CurveCreator<
		primitive::BezierCurve
	>::update(const glm::mat4& vp_matrix, glm::u8vec4 color) noexcept
	{
		const auto mouse_world_pos = math::uv_to_world(mouse_uv(), vp_matrix);

		switch (get_curve_action())
		{
		case CurveAction::None:
			return CurveContinue();

		case CurveAction::AddPoint:
			control_points.push_back(mouse_world_pos);
			return CurveContinue();

		case CurveAction::PopPoint:
			if (!control_points.empty()) control_points.pop_back();
			return CurveContinue();

		case CurveAction::Finish:
			return get_curve_with_mouse(vp_matrix, color).value();

		case CurveAction::Interrupt:
			return CurveInterrupt();
		}

		return CurveInterrupt();
	}

	template <>
	std::variant<CurveInterrupt, CurveContinue, primitive::CubicSpline> CurveCreator<
		primitive::CubicSpline
	>::update(const glm::mat4& vp_matrix, glm::u8vec4 color) noexcept
	{
		const auto mouse_world_pos = math::uv_to_world(mouse_uv(), vp_matrix);

		switch (get_curve_action())
		{
		case CurveAction::None:
			return CurveContinue();

		case CurveAction::AddPoint:
			control_points.push_back(mouse_world_pos);
			return CurveContinue();

		case CurveAction::PopPoint:
			if (!control_points.empty()) control_points.pop_back();
			return CurveContinue();

		case CurveAction::Finish:
			return get_curve_with_mouse(vp_matrix, color).value();

		case CurveAction::Interrupt:
			return CurveInterrupt();
		}

		return CurveInterrupt();
	}
}