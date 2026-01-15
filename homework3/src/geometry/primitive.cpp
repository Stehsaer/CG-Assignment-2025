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
}