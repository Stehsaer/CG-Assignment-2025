#include "geometry/primitive.hpp"
#include "geometry/control-point.hpp"
#include "geometry/vertex.hpp"
#include "math.hpp"

#include <glm/gtc/constants.hpp>
#include <imgui.h>
#include <ranges>

namespace primitive
{
	static constexpr float UI_LINE_WIDTH = 1.5f;
	static constexpr auto UI_LINE_COLOR = IM_COL32(200, 200, 200, 200);

	std::vector<LineVertex> Line::gen_vertices(this const Line& self) noexcept
	{
		return {
			{.position = self.begin.position, .color = self.color},
			{.position = self.end.position,   .color = self.color}
		};
	}

	void Line::draw_ui(this const Line& self, const glm::mat4& vp_matrix) noexcept
	{
		self.begin.draw(vp_matrix);
		self.end.draw(vp_matrix);
	}

	void Line::edit(this Line& self, const glm::mat4& vp_matrix) noexcept
	{
		self.draw_ui(vp_matrix);
		self.begin.drag(vp_matrix);
		self.end.drag(vp_matrix);
	}

	std::vector<LineVertex> Circle::gen_vertices(this const Circle& self) noexcept
	{
		const auto radius = glm::distance(self.center.position, self.border.position);

		const auto point_at_idx = [&self, radius](uint32_t idx) {
			const float angle = idx / static_cast<float>(SEGMENTS) * glm::two_pi<float>();
			return LineVertex{
				.position = self.center.position + glm::vec2(glm::cos(angle), glm::sin(angle)) * radius,
				.color = self.color
			};
		};

		return std::views::iota(0u, SEGMENTS + 1u)
			| std::views::transform(point_at_idx)
			| std::ranges::to<std::vector>();
	}

	void Circle::draw_ui(this const Circle& self, const glm::mat4& vp_matrix) noexcept
	{
		auto drawlist = ImGui::GetBackgroundDrawList();

		const auto& io = ImGui::GetIO();
		const auto viewport_size = glm::vec2(io.DisplaySize.x, io.DisplaySize.y);

		const auto center_viewport_pos = viewport_size * math::world_to_uv(self.center.position, vp_matrix);
		const auto border_viewport_pos = viewport_size * math::world_to_uv(self.border.position, vp_matrix);

		drawlist->AddLine(
			{center_viewport_pos.x, center_viewport_pos.y},
			{border_viewport_pos.x, border_viewport_pos.y},
			UI_LINE_COLOR,
			UI_LINE_WIDTH
		);

		self.center.draw(vp_matrix);
		self.border.draw(vp_matrix);
	}

	void Circle::edit(this Circle& self, const glm::mat4& vp_matrix) noexcept
	{
		self.draw_ui(vp_matrix);
		self.center.drag(vp_matrix);
		self.border.drag(vp_matrix);
	}

	std::vector<LineVertex> BezierCurve::gen_vertices(this const BezierCurve& self) noexcept
	{
		if (self.control_points.size() < 2) return {};

		std::vector<glm::vec2> buffer(self.control_points.size());

		const auto point_at_t = [&self, &buffer](float t) {
			std::ranges::copy(
				self.control_points | std::views::transform(&ControlPoint::position),
				buffer.begin()
			);

			for (size_t r = self.control_points.size() - 1; r > 0; --r)
				for (auto [p1, p2] : std::views::adjacent<2>(buffer | std::views::take(r + 1)))
					p1 = glm::mix(p1, p2, t);

			return LineVertex{.position = buffer[0], .color = self.color};
		};

		return std::views::iota(0u, SEGMENTS + 1u)
			| std::views::transform([&point_at_t](uint32_t i) {
				   const float t = i / static_cast<float>(SEGMENTS);
				   return point_at_t(t);
			   })
			| std::ranges::to<std::vector>();
	}

	void BezierCurve::draw_ui(this const BezierCurve& self, const glm::mat4& vp_matrix) noexcept
	{
		auto& io = ImGui::GetIO();
		auto drawlist = ImGui::GetBackgroundDrawList();
		const auto viewport_size = glm::vec2(io.DisplaySize.x, io.DisplaySize.y);

		for (const auto [vp1, vp2] :
			 self.control_points | std::views::transform([&vp_matrix, viewport_size](const ControlPoint& cp) {
				 const auto uv = math::world_to_uv(cp.position, vp_matrix);
				 return uv * viewport_size;
			 }) | std::views::adjacent<2>)
			drawlist->AddLine({vp1.x, vp1.y}, {vp2.x, vp2.y}, UI_LINE_COLOR, UI_LINE_WIDTH);

		for (const auto& cp : self.control_points) cp.draw(vp_matrix);
	}

	void BezierCurve::edit(this BezierCurve& self, const glm::mat4& vp_matrix) noexcept
	{
		self.draw_ui(vp_matrix);
		for (auto& cp : self.control_points) cp.drag(vp_matrix);
	}

	glm::vec2 CubicSpline::point_at(this const CubicSpline& self, float t) noexcept
	{
		const uint32_t p0_idx = glm::floor(t);
		const float u = t - float(p0_idx);
		const float u2 = u * u;
		const float u3 = u2 * u;

		const auto p0 = self.control_points[p0_idx].position;
		const auto p1 = self.control_points[p0_idx + 1].position;
		const auto p2 = self.control_points[p0_idx + 2].position;
		const auto p3 = self.control_points[p0_idx + 3].position;

		const auto p0_coeff_6 = -u3 + 3 * u2 - 3 * u + 1;
		const auto p1_coeff_6 = 3 * u3 - 6 * u2 + 4;
		const auto p2_coeff_6 = -3 * u3 + 3 * u2 + 3 * u + 1;
		const auto p3_coeff_6 = u3;

		return ((p0 * p0_coeff_6) + (p1 * p1_coeff_6) + (p2 * p2_coeff_6) + (p3 * p3_coeff_6)) / 6.0f;
	}

	std::vector<LineVertex> CubicSpline::gen_vertices(this const CubicSpline& self) noexcept
	{
		if (self.control_points.size() < 4) return {};

		return std::views::iota(0u, SEGMENTS_PER_PATH * (self.control_points.size() - 3))
			| std::views::transform([&self](uint32_t i) {
				   const float t = i / static_cast<float>(SEGMENTS_PER_PATH);
				   return LineVertex{.position = self.point_at(t), .color = self.color};
			   })
			| std::ranges::to<std::vector>();
	}

	void CubicSpline::draw_ui(this const CubicSpline& self, const glm::mat4& vp_matrix) noexcept
	{
		auto& io = ImGui::GetIO();
		auto drawlist = ImGui::GetBackgroundDrawList();
		const auto viewport_size = glm::vec2(io.DisplaySize.x, io.DisplaySize.y);

		for (const auto [vp1, vp2] :
			 self.control_points | std::views::transform([&vp_matrix, viewport_size](const ControlPoint& cp) {
				 const auto uv = math::world_to_uv(cp.position, vp_matrix);
				 return uv * viewport_size;
			 }) | std::views::adjacent<2>)
			drawlist->AddLine({vp1.x, vp1.y}, {vp2.x, vp2.y}, UI_LINE_COLOR, UI_LINE_WIDTH);

		for (const auto& cp : self.control_points) cp.draw(vp_matrix);
	}

	void CubicSpline::edit(this CubicSpline& self, const glm::mat4& vp_matrix) noexcept
	{
		self.draw_ui(vp_matrix);
		for (auto& cp : self.control_points) cp.drag(vp_matrix);
	}
}