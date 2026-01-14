#include "gltf/mesh.hpp"
#include "gltf/detail/mesh/optimize.hpp"
#include "gltf/detail/mesh/raw-primitive-list.hpp"

#include "graphics/util/quick-create.hpp"
#include "util/as-byte.hpp"
#include <algorithm>
#include <ranges>

namespace gltf
{
	using namespace detail::mesh;

	static constexpr float vertex_eq_thres = 0.9999f;

	bool Vertex::operator==(const Vertex& other) const noexcept
	{
		const bool position_equal = position == other.position;
		const bool normal_equal = glm::dot(normal, other.normal) >= vertex_eq_thres;
		const bool texcoord_equal = texcoord == other.texcoord;
		const bool tangent_equal = glm::dot(tangent, other.tangent) >= vertex_eq_thres;

		return position_equal && normal_equal && texcoord_equal && tangent_equal;
	}

	bool RiggedVertex::operator==(const RiggedVertex& other) const noexcept
	{
		const bool position_equal = position == other.position;
		const bool normal_equal = glm::dot(normal, other.normal) >= vertex_eq_thres;
		const bool texcoord_equal = texcoord == other.texcoord;
		const bool tangent_equal = glm::dot(tangent, other.tangent) >= vertex_eq_thres;
		const bool joint_indices_equal = joint_indices == other.joint_indices;
		const bool joint_weights_equal =
			glm::distance(joint_weights, other.joint_weights) <= 1 - vertex_eq_thres;

		return position_equal
			&& normal_equal
			&& texcoord_equal
			&& tangent_equal
			&& joint_indices_equal
			&& joint_weights_equal;
	}

	bool ShadowVertex::operator==(const ShadowVertex& other) const noexcept
	{
		const bool position_equal = position == other.position;
		const bool texcoord_equal = texcoord == other.texcoord;

		return position_equal && texcoord_equal;
	}

	bool RiggedShadowVertex::operator==(const RiggedShadowVertex& other) const noexcept
	{
		const bool position_equal = position == other.position;
		const bool texcoord_equal = texcoord == other.texcoord;
		const bool joint_indices_equal = joint_indices == other.joint_indices;
		const bool joint_weights_equal =
			glm::distance(joint_weights, other.joint_weights) <= 1 - vertex_eq_thres;

		return position_equal && texcoord_equal && joint_indices_equal && joint_weights_equal;
	}

	ShadowVertex ShadowVertex::from_vertex(const Vertex& vertex) noexcept
	{
		return ShadowVertex{
			.position = vertex.position,
			.texcoord = vertex.texcoord,
		};
	}

	RiggedShadowVertex RiggedShadowVertex::from_rigged_vertex(const RiggedVertex& vertex) noexcept
	{
		return RiggedShadowVertex{
			.position = vertex.position,
			.texcoord = vertex.texcoord,
			.joint_indices = vertex.joint_indices,
			.joint_weights = vertex.joint_weights,
		};
	}

	std::expected<Primitive, util::Error> Primitive::from_tinygltf(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive
	) noexcept
	{
		if (primitive.attributes.contains("JOINTS_0") || primitive.attributes.contains("WEIGHTS_0"))
			return util::Error(
				"Primitive contains skinning attributes, rigged primitive class should be used instead"
			);

		/* Acquire & Process Vertex List */

		auto vertex_list_result = get_primitive_list(model, primitive);
		if (!vertex_list_result)
			return vertex_list_result.error().forward("Get primitive vertex list failed");

		auto [optimized_vertices, optimized_indices] = optimize_primitive(*vertex_list_result);
		auto [optimized_shadow_vertices, optimized_shadow_indices] = optimize_primitive(
			*vertex_list_result
			| std::views::transform(&ShadowVertex::from_vertex)
			| std::ranges::to<std::vector>()
		);

		/* Calculate Min/Max */

		auto position_min = std::ranges::fold_left(
			optimized_shadow_vertices | std::views::transform(&ShadowVertex::position),
			glm::vec3(std::numeric_limits<float>::max()),
			[](const glm::vec3& a, const glm::vec3& b) { return glm::min(a, b); }
		);

		auto position_max = std::ranges::fold_left(
			optimized_shadow_vertices | std::views::transform(&ShadowVertex::position),
			glm::vec3(std::numeric_limits<float>::lowest()),
			[](const glm::vec3& a, const glm::vec3& b) { return glm::max(a, b); }
		);

		/* Clamp Minimum Dimension */

		auto size = position_max - position_min;
		const auto min_dim = std::min({size.x, size.y, size.z});
		const auto max_dim = std::max({size.x, size.y, size.z});
		if (min_dim < 0.0001f * max_dim)
		{
			const auto center = (position_min + position_max) * 0.5f;
			const auto min_extent = 0.0005f * max_dim;
			position_min = glm::min(position_min, center - glm::vec3(min_extent));
			position_max = glm::max(position_max, center + glm::vec3(min_extent));
		}

		return Primitive{
			.vertices = std::move(optimized_vertices),
			.indices = std::move(optimized_indices),
			.shadow_vertices = std::move(optimized_shadow_vertices),
			.shadow_indices = std::move(optimized_shadow_indices),
			.material = primitive.material == -1 ? std::nullopt : std::optional<uint32_t>(primitive.material),
			.position_min = position_min,
			.position_max = position_max,
		};
	}

	std::expected<RiggedPrimitive, util::Error> RiggedPrimitive::from_tinygltf(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive
	) noexcept
	{
		/* Acquire & Process Vertex List */

		auto vertex_list_result = get_rigged_primitive_list(model, primitive);
		if (!vertex_list_result)
			return vertex_list_result.error().forward("Get rigged primitive vertex list failed");

		auto [optimized_vertices, optimized_indices] = optimize_primitive(*vertex_list_result);
		auto [optimized_shadow_vertices, optimized_shadow_indices] = optimize_primitive(
			*vertex_list_result
			| std::views::transform(&RiggedShadowVertex::from_rigged_vertex)
			| std::ranges::to<std::vector>()
		);

		/* Calculate Min/Max */

		auto position_min = std::ranges::fold_left(
			optimized_shadow_vertices | std::views::transform(&RiggedShadowVertex::position),
			glm::vec3(std::numeric_limits<float>::max()),
			[](const glm::vec3& a, const glm::vec3& b) { return glm::min(a, b); }
		);

		auto position_max = std::ranges::fold_left(
			optimized_shadow_vertices | std::views::transform(&RiggedShadowVertex::position),
			glm::vec3(std::numeric_limits<float>::lowest()),
			[](const glm::vec3& a, const glm::vec3& b) { return glm::max(a, b); }
		);

		/* Clamp Minimum Dimension */

		auto size = position_max - position_min;
		const auto min_dim = std::min({size.x, size.y, size.z});
		const auto max_dim = std::max({size.x, size.y, size.z});
		if (min_dim < 0.0001f * max_dim)
		{
			const auto center = (position_min + position_max) * 0.5f;
			const auto min_extent = 0.0005f * max_dim;
			position_min = glm::min(position_min, center - glm::vec3(min_extent));
			position_max = glm::max(position_max, center + glm::vec3(min_extent));
		}

		return RiggedPrimitive{
			.vertices = std::move(optimized_vertices),
			.indices = std::move(optimized_indices),
			.shadow_vertices = std::move(optimized_shadow_vertices),
			.shadow_indices = std::move(optimized_shadow_indices),
			.material = primitive.material == -1 ? std::nullopt : std::optional<uint32_t>(primitive.material),
			.position_min = position_min,
			.position_max = position_max
		};
	}

	std::expected<PrimitiveGPU, util::Error> PrimitiveGPU::from_primitive(
		SDL_GPUDevice* device,
		const Primitive& primitive
	) noexcept
	{
		auto vertex_buffer = graphics::create_buffer_from_data(
			device,
			{.vertex = true},
			util::as_bytes(primitive.vertices),
			"GLTF Vertex Buffer"
		);

		auto index_buffer = graphics::create_buffer_from_data(
			device,
			{.index = true},
			util::as_bytes(primitive.indices),
			"GLTF Index Buffer"
		);

		auto shadow_vertex_buffer = graphics::create_buffer_from_data(
			device,
			{.vertex = true},
			util::as_bytes(primitive.shadow_vertices),
			"GLTF Shadow Vertex Buffer"
		);

		auto shadow_index_buffer = graphics::create_buffer_from_data(
			device,
			{.index = true},
			util::as_bytes(primitive.shadow_indices),
			"GLTF Shadow Index Buffer"
		);

		if (!vertex_buffer) return vertex_buffer.error().forward("Create vertex buffer failed");
		if (!index_buffer) return index_buffer.error().forward("Create index buffer failed");
		if (!shadow_vertex_buffer)
			return shadow_vertex_buffer.error().forward("Create position vertex buffer failed");
		if (!shadow_index_buffer)
			return shadow_index_buffer.error().forward("Create position index buffer failed");

		return PrimitiveGPU{
			.index_count = static_cast<uint32_t>(primitive.indices.size()),

			.vertex_buffer = std::move(*vertex_buffer),
			.index_buffer = std::move(*index_buffer),
			.shadow_vertex_buffer = std::move(*shadow_vertex_buffer),
			.shadow_index_buffer = std::move(*shadow_index_buffer),

			.material = primitive.material,
			.position_min = primitive.position_min,
			.position_max = primitive.position_max,
			.rigged = false
		};
	}

	std::expected<PrimitiveGPU, util::Error> PrimitiveGPU::from_rigged_primitive(
		SDL_GPUDevice* device,
		const RiggedPrimitive& primitive
	) noexcept
	{
		auto vertex_buffer = graphics::create_buffer_from_data(
			device,
			{.vertex = true},
			util::as_bytes(primitive.vertices),
			"GLTF Rigged Vertex Buffer"
		);

		auto index_buffer = graphics::create_buffer_from_data(
			device,
			{.index = true},
			util::as_bytes(primitive.indices),
			"GLTF Rigged Index Buffer"
		);

		auto shadow_vertex_buffer = graphics::create_buffer_from_data(
			device,
			{.vertex = true},
			util::as_bytes(primitive.shadow_vertices),
			"GLTF Rigged Shadow Vertex Buffer"
		);

		auto shadow_index_buffer = graphics::create_buffer_from_data(
			device,
			{.index = true},
			util::as_bytes(primitive.shadow_indices),
			"GLTF Rigged Shadow Index Buffer"
		);

		if (!vertex_buffer) return vertex_buffer.error().forward("Create vertex buffer failed");
		if (!index_buffer) return index_buffer.error().forward("Create index buffer failed");
		if (!shadow_vertex_buffer)
			return shadow_vertex_buffer.error().forward("Create position vertex buffer failed");
		if (!shadow_index_buffer)
			return shadow_index_buffer.error().forward("Create position index buffer failed");

		return PrimitiveGPU{
			.index_count = static_cast<uint32_t>(primitive.indices.size()),

			.vertex_buffer = std::move(*vertex_buffer),
			.index_buffer = std::move(*index_buffer),
			.shadow_vertex_buffer = std::move(*shadow_vertex_buffer),
			.shadow_index_buffer = std::move(*shadow_index_buffer),

			.material = primitive.material,
			.position_min = primitive.position_min,
			.position_max = primitive.position_max,
			.rigged = true
		};
	}

	std::expected<Mesh, util::Error> Mesh::from_tinygltf(
		const tinygltf::Model& model,
		const tinygltf::Mesh& mesh
	) noexcept
	{
		std::vector<Primitive> primitives;
		std::vector<RiggedPrimitive> rigged_primitives;
		primitives.reserve(mesh.primitives.size());
		rigged_primitives.reserve(mesh.primitives.size());

		for (const auto& primitive : mesh.primitives)
		{
			if (primitive.attributes.contains("JOINTS_0") || primitive.attributes.contains("WEIGHTS_0"))
			{
				auto rigged_primitive_result = RiggedPrimitive::from_tinygltf(model, primitive);
				if (!rigged_primitive_result)
					return rigged_primitive_result.error().forward("Create Rigged_Primitive failed");

				rigged_primitives.emplace_back(std::move(*rigged_primitive_result));
			}
			else
			{
				auto primitive_result = Primitive::from_tinygltf(model, primitive);
				if (!primitive_result) return primitive_result.error().forward("Create Primitive failed");

				primitives.emplace_back(std::move(*primitive_result));
			}
		}

		return Mesh{.primitives = std::move(primitives), .rigged_primitives = std::move(rigged_primitives)};
	}

	std::expected<MeshGPU, util::Error> MeshGPU::from_mesh(SDL_GPUDevice* device, const Mesh& mesh) noexcept
	{
		std::vector<PrimitiveGPU> primitives;
		primitives.reserve(mesh.primitives.size() + mesh.rigged_primitives.size());

		for (const auto& primitive : mesh.primitives)
		{
			auto primitive_result = PrimitiveGPU::from_primitive(device, primitive);
			if (!primitive_result) return primitive_result.error().forward("Create Primitive_gpu failed");

			primitives.emplace_back(std::move(*primitive_result));
		}

		for (const auto& rigged_primitive : mesh.rigged_primitives)
		{
			auto rigged_primitive_result = PrimitiveGPU::from_rigged_primitive(device, rigged_primitive);
			if (!rigged_primitive_result)
				return rigged_primitive_result.error().forward("Create Rigged_Primitive_gpu failed");

			primitives.emplace_back(std::move(*rigged_primitive_result));
		}

		return MeshGPU{.primitives = std::move(primitives)};
	}
}