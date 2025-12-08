#include "gltf/detail/mesh/optimize.hpp"

#include <meshoptimizer.h>
#include <ranges>

namespace gltf::detail::mesh
{
	static std::pair<std::vector<Vertex>, std::vector<uint32_t>> remap(
		const std::vector<Vertex>& vertices
	) noexcept
	{
		std::vector<uint32_t> remap_table(vertices.size());
		const auto vertex_count = meshopt_generateVertexRemapCustom(
			remap_table.data(),
			nullptr,
			vertices.size(),
			&vertices[0].position.x,
			vertices.size(),
			sizeof(Vertex),
			[&vertices](const uint32_t& a_idx, const uint32_t& b_idx) {
				constexpr float thres = 0.9999f;

				const auto& a = vertices[a_idx];
				const auto& b = vertices[b_idx];

				const bool position_equal = a.position == b.position;
				const bool normal_equal = glm::dot(a.normal, b.normal) >= thres;
				const bool texcoord_equal = a.texcoord == b.texcoord;
				const bool tangent_equal = glm::dot(a.tangent, b.tangent) >= thres;

				return position_equal && normal_equal && texcoord_equal && tangent_equal;
			}
		);

		std::vector<Vertex> remapped_vertices(vertex_count);
		std::vector<uint32_t> remapped_indices(vertices.size());

		meshopt_remapVertexBuffer(
			remapped_vertices.data(),
			vertices.data(),
			vertices.size(),
			sizeof(Vertex),
			remap_table.data()
		);

		meshopt_remapIndexBuffer(remapped_indices.data(), nullptr, vertices.size(), remap_table.data());

		return {std::move(remapped_vertices), std::move(remapped_indices)};
	}

	std::pair<std::vector<Vertex>, std::vector<uint32_t>> optimize_primitive(
		const std::vector<Vertex>& vertices
	) noexcept
	{
		auto [remapped_vertices, remapped_indices] = remap(vertices);

		meshopt_optimizeVertexCache(
			remapped_indices.data(),
			remapped_indices.data(),
			remapped_indices.size(),
			remapped_vertices.size()
		);

		meshopt_optimizeOverdraw(
			remapped_indices.data(),
			remapped_indices.data(),
			remapped_indices.size(),
			&remapped_vertices[0].position.x,
			remapped_vertices.size(),
			sizeof(Vertex),
			1.05f
		);

		return {std::move(remapped_vertices), std::move(remapped_indices)};
	}

	std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> optimize_position_only_primitive(
		const std::vector<Vertex>& vertices
	) noexcept
	{
		auto position_only_vertices =
			vertices | std::views::transform(&Vertex::position) | std::ranges::to<std::vector>();

		std::vector<uint32_t> remap_table(vertices.size());
		const auto vertex_count = meshopt_generateVertexRemap(
			remap_table.data(),
			nullptr,
			vertices.size(),
			&position_only_vertices[0].x,
			vertices.size(),
			sizeof(glm::vec3)
		);

		std::vector<glm::vec3> remapped_vertices(vertex_count);
		std::vector<uint32_t> remapped_indices(vertices.size());

		meshopt_remapVertexBuffer(
			remapped_vertices.data(),
			vertices.data(),
			vertices.size(),
			sizeof(glm::vec3),
			remap_table.data()
		);
		meshopt_remapIndexBuffer(remapped_indices.data(), nullptr, vertices.size(), remap_table.data());

		meshopt_optimizeVertexCache(
			remapped_indices.data(),
			remapped_indices.data(),
			remapped_indices.size(),
			remapped_vertices.size()
		);

		meshopt_optimizeOverdraw(
			remapped_indices.data(),
			remapped_indices.data(),
			remapped_indices.size(),
			&remapped_vertices[0].x,
			remapped_vertices.size(),
			sizeof(glm::vec3),
			1.05f
		);

		return {std::move(remapped_vertices), std::move(remapped_indices)};
	}
}