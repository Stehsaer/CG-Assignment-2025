#include "graphics/smallest-bound.hpp"
#include <algorithm>
#include <glm/ext/matrix_transform.hpp>

#include <glm/fwd.hpp>
#include <ranges>

namespace graphics
{
	// Get a view matrix that looks in the direction of `view_dir`. Automatically chooses an up vector.
	static glm::mat4 get_view_matrix(const glm::vec3& view_dir) noexcept
	{
		if (glm::abs(glm::dot(glm::normalize(view_dir), glm::vec3(0.0f, 1.0f, 0.0f))) > 0.999f)
			return glm::lookAt(glm::vec3(0.0f), view_dir, glm::vec3(1.0f, 0.0f, 0.0f));
		else
			return glm::lookAt(glm::vec3(0.0f), view_dir, glm::vec3(0.0f, 1.0f, 0.0f));
	}

	// Project 3D points to 2D using the given view matrix.
	static std::array<glm::vec2, 8> project_points_to_2d(
		const std::array<glm::vec3, 8>& points,
		const glm::mat4& view_matrix
	) noexcept
	{
		std::array<glm::vec2, 8> projected_points;
		for (auto [src, dst] : std::views::zip(points, projected_points))
		{
			const glm::vec4 dst_homo = view_matrix * glm::vec4(src, 1.0f);
			const glm::vec4 dst_coord = dst_homo / dst_homo.w;
			dst = {dst_coord.x, dst_coord.y};
		}
		return projected_points;
	}

	// Return list of points, the anchor at the front, and 7 other points follows it in angle order
	static std::vector<glm::vec2> sort_angle(const std::array<glm::vec2, 8>& arr) noexcept
	{
		const glm::vec2 anchor = std::ranges::min(arr, std::less(), &glm::vec2::y);

		auto vertices = arr
			| std::views::filter([anchor](const auto& elem) { return elem != anchor; })
			| std::ranges::to<std::vector>();
		std::ranges::sort(vertices, std::less(), [anchor](auto elem) {
			const auto vec = elem - anchor;
			return std::atan2(vec.y, vec.x);
		});

		vertices.emplace(vertices.begin(), anchor);

		return vertices;
	}

	// Calculate convex hull
	static std::vector<glm::vec2> calc_convex_hull(std::vector<glm::vec2> sorted_points) noexcept
	{
		assert(sorted_points.size() >= 3);

		std::vector<glm::vec2> convex_hull_stack;
		convex_hull_stack.reserve(16);

		while (!sorted_points.empty())
		{
			if (convex_hull_stack.size() < 2)
			{
				convex_hull_stack.push_back(sorted_points.front());
				sorted_points.erase(sorted_points.begin());
				continue;
			}

			const auto x0 = convex_hull_stack[convex_hull_stack.size() - 2];
			const auto x1 = convex_hull_stack[convex_hull_stack.size() - 1];
			const auto x2 = sorted_points.front();

			const auto cross = (x1.x - x0.x) * (x2.y - x0.y) - (x1.y - x0.y) * (x2.x - x0.x);

			if (cross > 0)  // Left turn, put x2 to stack
			{
				convex_hull_stack.push_back(x2);
				sorted_points.erase(sorted_points.begin());
			}
			else  // Right turn or colinear, pop x1 from stack
			{
				convex_hull_stack.pop_back();
			}
		}

		convex_hull_stack.push_back(convex_hull_stack.front());
		return convex_hull_stack;
	}

	SmallestBound find_smallest_bound(
		const std::array<glm::vec3, 8>& frustum_corners,
		const glm::vec3& view_dir
	) noexcept
	{
		/* Find Convex Hull */
		const auto view_matrix = get_view_matrix(view_dir);
		const auto projected_points = project_points_to_2d(frustum_corners, view_matrix);
		auto sorted_points = sort_angle(projected_points);
		const auto convex_hull = calc_convex_hull(std::move(sorted_points));

		/* Find Smallest Bound */

		const auto max_index = std::get<0>(std::ranges::max(
			convex_hull
			| std::views::adjacent<2>
			| std::views::transform([&convex_hull](const auto& edge) {
				  const auto [x0, x1] = edge;
				  return std::ranges::max_element(
							 convex_hull,
							 std::less(),
							 [v1 = x1 - x0, x0](const glm::vec2& p) { return glm::dot(v1, p - x0); }
						 )
					  - convex_hull.begin();
			  })
			| std::views::enumerate
		));

		/* Calculate Rotate */

		const auto vec = convex_hull[max_index + 1] - convex_hull[max_index];
		const auto angle = std::atan2(vec.y, vec.x);
		const glm::mat4 rotation_mat = glm::rotate(glm::mat4(1.0f), -angle, glm::vec3(0.0f, 0.0f, 1.0f));

		/* Apply Rotation */

		auto rotated_points =
			convex_hull | std::views::transform([&rotation_mat](const auto& p) {
				const glm::vec4 rp = rotation_mat * glm::vec4(p, 0.0f, 1.0f);
				return glm::vec2(rp.x, rp.y);
			});

		/* Minmax */

		const auto min = std::ranges::fold_left(
			rotated_points,
			glm::vec2(std::numeric_limits<float>::max()),
			[](const auto& acc, const auto& p) { return glm::min(acc, p); }
		);

		const auto max = std::ranges::fold_left(
			rotated_points,
			glm::vec2(std::numeric_limits<float>::lowest()),
			[](const auto& acc, const auto& p) { return glm::max(acc, p); }
		);

		return SmallestBound{
			.view_matrix = rotation_mat * view_matrix,
			.left = min.x,
			.right = max.x,
			.top = min.y,
			.bottom = max.y
		};
	}
}