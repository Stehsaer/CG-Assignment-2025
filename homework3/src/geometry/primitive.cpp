#include "geometry/primitive.hpp"
#include "geometry/vertex.hpp"

#include <glm/gtc/constants.hpp>
#include <ranges>

namespace primitive
{
	std::vector<LineVertex> Line::gen_vertices(this const Line& self) noexcept
	{
		return {self.begin, self.end};
	}

	std::vector<LineVertex> Circle::gen_vertices(this const Circle& self) noexcept
	{
		const auto point_at_idx = [&self](uint32_t idx) {
			const float angle = idx / static_cast<float>(SEGMENTS) * glm::two_pi<float>();
			return LineVertex{
				.position = self.center + glm::vec2(glm::cos(angle), glm::sin(angle)) * self.radius,
				.color = self.color
			};
		};

		return std::views::iota(0u, SEGMENTS + 1u)
			| std::views::transform(point_at_idx)
			| std::ranges::to<std::vector>();
	}

	std::vector<LineVertex> BezierCurve::gen_vertices(this const BezierCurve& self) noexcept
	{
		if (self.control_points.size() < 2) return {};

		std::vector<glm::vec2> buffer(self.control_points.size());

		const auto point_at_t = [&self, &buffer](float t) {
			std::ranges::copy(self.control_points, buffer.begin());

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

	std::vector<LineVertex> CubicSpline::gen_vertices(this const CubicSpline& self) noexcept
	{
		if (self.control_points.size() < 4) return {};

		const auto point_at_t = [&self](float t) -> glm::vec2 {
			const uint32_t p0_idx = glm::floor(t);
			const float u = t - float(p0_idx);
			const float u2 = u * u;
			const float u3 = u2 * u;

			const auto p0 = self.control_points[p0_idx];
			const auto p1 = self.control_points[p0_idx + 1];
			const auto p2 = self.control_points[p0_idx + 2];
			const auto p3 = self.control_points[p0_idx + 3];

			const auto p0_coeff_6 = -u3 + 3 * u2 - 3 * u + 1;
			const auto p1_coeff_6 = 3 * u3 - 6 * u2 + 4;
			const auto p2_coeff_6 = -3 * u3 + 3 * u2 + 3 * u + 1;
			const auto p3_coeff_6 = u3;

			return ((p0 * p0_coeff_6) + (p1 * p1_coeff_6) + (p2 * p2_coeff_6) + (p3 * p3_coeff_6)) / 6.0f;
		};

		return std::views::iota(0u, SEGMENTS_PER_PATH * (self.control_points.size() - 3))
			| std::views::transform([&point_at_t, &self](uint32_t i) {
				   const float t = i / static_cast<float>(SEGMENTS_PER_PATH);
				   return LineVertex{.position = point_at_t(t), .color = self.color};
			   })
			| std::ranges::to<std::vector>();
	}
}