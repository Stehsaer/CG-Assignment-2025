#pragma once

#include "util/error.hpp"
#include <expected>
#include <glm/glm.hpp>
#include <optional>
#include <ranges>
#include <tiny_gltf.h>
#include <vector>

namespace gltf::detail::mesh
{
	///
	/// @brief Unpack vertices from index buffer
	///
	/// @tparam T Type of vertex
	/// @param vertices Input vertices
	/// @param indices Optional index buffer
	/// @return Unpacked vertices if indices are provided, else original vertices
	///
	template <typename T>
	std::expected<std::vector<T>, util::Error> unpack_from_indices(
		const std::vector<T>& vertices,
		const std::optional<std::vector<uint32_t>>& indices
	) noexcept
	{
		if (!indices.has_value()) return vertices;

		auto find_out_of_bounds =
			std::ranges::find_if(*indices, [size = vertices.size()](const uint32_t& idx) {
				return std::cmp_greater_equal(idx, size);
			});
		if (find_out_of_bounds != indices->end())
			return util::Error(
				std::format(
					"Index {} out of bounds at index_buffer[{}] (vertex count {})",
					*find_out_of_bounds,
					find_out_of_bounds - indices->begin(),
					vertices.size()
				)
			);

		std::vector<T> remapped_vertices(indices->size());
		for (const auto i : std::views::iota(0zu, indices->size()))
		{
			const auto& index = (*indices)[i];
			remapped_vertices[i] = std::cmp_greater_equal(index, vertices.size()) ? T(0.0f) : vertices[index];
		}

		return remapped_vertices;
	}

	///
	/// @brief Flatten vertices into triangle list form
	/// @note Guaranteed to return triangle list form, with 3 vertices per triangle
	///
	/// @tparam T Type of vertex
	/// @param vertices Input vertices
	/// @param mode Tinygltf primitive mode
	/// @return Expected flattened vertices or error
	///
	template <typename T>
	std::expected<std::vector<T>, util::Error> rearrange_vertices(
		const std::vector<T>& vertices,
		int mode
	) noexcept
	{
		if (mode == TINYGLTF_MODE_TRIANGLES)
		{
			if (vertices.size() % 3 != 0)
				return util::Error("Triangle primitive vertex count should be a multiple of 3");

			return std::move(vertices);
		}

		if (mode == TINYGLTF_MODE_TRIANGLE_FAN)
		{
			if (vertices.size() < 3)
				return util::Error("Triangle fan primitive vertex count should be at least 3");
			const auto tri_count = vertices.size() - 2;

			std::vector<T> result;

			for (const auto idx : std::views::iota(0zu, tri_count))
			{
				result.push_back(vertices[idx + 1]);
				result.push_back(vertices[idx + 2]);
				result.push_back(vertices[0]);
			}

			return std::move(result);
		}

		if (mode == TINYGLTF_MODE_TRIANGLE_STRIP)
		{
			if (vertices.size() < 3)
				return util::Error("Triangle strip primitive vertex count should be at least 3");
			const auto tri_count = vertices.size() - 2;

			std::vector<T> result;

			for (const auto idx : std::views::iota(0zu, tri_count))
			{
				result.push_back(vertices[idx]);
				result.push_back(vertices[idx + (1 + idx % 2)]);
				result.push_back(vertices[idx + (2 - idx % 2)]);
			}

			return std::move(result);
		}

		return util::Error("Unsupported primitive mode");
	}
}