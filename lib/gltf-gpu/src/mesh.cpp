#include "gltf/mesh.hpp"
#include "gltf/detail/mesh/optimize.hpp"
#include "gltf/detail/mesh/raw-primitive-list.hpp"

#include <algorithm>
#include <graphic/util/tool.hpp>
#include <util/as-byte.hpp>

namespace gltf
{
	using namespace detail::mesh;

	std::expected<Primitive, util::Error> Primitive::from_tinygltf(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive
	) noexcept
	{
		auto vertex_list_result = get_primitive_list(model, primitive);
		if (!vertex_list_result)
			return vertex_list_result.error().propagate("Get primitive vertex list failed");

		auto [optimized_vertices, optimized_indices] = optimize_primitive(*vertex_list_result);
		auto [optimized_position_only_vertices, position_only_indices] =
			optimize_position_only_primitive(*vertex_list_result);

		const auto position_min = std::ranges::fold_left(
			optimized_position_only_vertices,
			glm::vec3(std::numeric_limits<float>::max()),
			[](const glm::vec3& a, const glm::vec3& b) { return glm::min(a, b); }
		);

		const auto position_max = std::ranges::fold_left(
			optimized_position_only_vertices,
			glm::vec3(std::numeric_limits<float>::lowest()),
			[](const glm::vec3& a, const glm::vec3& b) { return glm::max(a, b); }
		);

		return Primitive{
			.vertices = std::move(optimized_vertices),
			.indices = std::move(optimized_indices),
			.position_vertices = std::move(optimized_position_only_vertices),
			.position_indices = std::move(position_only_indices),
			.material = primitive.material == -1 ? std::nullopt : std::optional<uint32_t>(primitive.material),
			.position_min = position_min,
			.position_max = position_max,
		};
	}

	std::expected<Primitive_gpu, util::Error> Primitive_gpu::from_primitive(
		SDL_GPUDevice* device,
		const Primitive& primitive
	) noexcept
	{
		auto vertex_buffer =
			graphic::create_buffer_with_data(device, {.vertex = true}, util::as_bytes(primitive.vertices));

		auto index_buffer =
			graphic::create_buffer_with_data(device, {.index = true}, util::as_bytes(primitive.indices));

		auto position_vertex_buffer = graphic::create_buffer_with_data(
			device,
			{.vertex = true},
			util::as_bytes(primitive.position_vertices)
		);

		auto position_index_buffer = graphic::create_buffer_with_data(
			device,
			{.index = true},
			util::as_bytes(primitive.position_indices)
		);

		if (!vertex_buffer) return vertex_buffer.error().propagate("Create vertex buffer failed");
		if (!index_buffer) return index_buffer.error().propagate("Create index buffer failed");
		if (!position_vertex_buffer)
			return position_vertex_buffer.error().propagate("Create position vertex buffer failed");
		if (!position_index_buffer)
			return position_index_buffer.error().propagate("Create position index buffer failed");

		return Primitive_gpu{
			.index_count = static_cast<uint32_t>(primitive.indices.size()),

			.vertex_buffer = std::move(*vertex_buffer),
			.index_buffer = std::move(*index_buffer),
			.position_vertex_buffer = std::move(*position_vertex_buffer),
			.position_index_buffer = std::move(*position_index_buffer),

			.material = primitive.material,
			.position_min = primitive.position_min,
			.position_max = primitive.position_max
		};
	}

	std::expected<Mesh, util::Error> Mesh::from_tinygltf(
		const tinygltf::Model& model,
		const tinygltf::Mesh& mesh
	) noexcept
	{
		std::vector<Primitive> primitives;
		primitives.reserve(mesh.primitives.size());

		for (const auto& primitive : mesh.primitives)
		{
			auto primitive_result = Primitive::from_tinygltf(model, primitive);
			if (!primitive_result) return primitive_result.error().propagate("Create Primitive failed");

			primitives.emplace_back(std::move(*primitive_result));
		}

		return Mesh{.primitives = std::move(primitives)};
	}

	std::expected<Mesh_gpu, util::Error> Mesh_gpu::from_mesh(SDL_GPUDevice* device, const Mesh& mesh) noexcept
	{
		std::vector<Primitive_gpu> primitives;
		primitives.reserve(mesh.primitives.size());

		for (const auto& primitive : mesh.primitives)
		{
			auto primitive_result = Primitive_gpu::from_primitive(device, primitive);
			if (!primitive_result) return primitive_result.error().propagate("Create Primitive_gpu failed");

			primitives.emplace_back(std::move(*primitive_result));
		}

		return Mesh_gpu{.primitives = std::move(primitives)};
	}
}