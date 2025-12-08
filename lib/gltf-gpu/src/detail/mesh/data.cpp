#include "gltf/detail/mesh/data.hpp"
#include "gltf/accessor.hpp"
#include "gltf/detail/mesh/topology.hpp"

#include <util/find.hpp>

namespace gltf::detail::mesh
{
	std::expected<std::vector<glm::vec3>, util::Error> get_primitive_position_raw(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive
	) noexcept
	{
		const auto position_accessor_idx = util::find_map(primitive.attributes, "POSITION");

		if (!position_accessor_idx) return util::Error("Primitive has no POSITION attribute");
		if (position_accessor_idx->get() < 0) return util::Error("Primitive POSITION has no accessor");
		if (std::cmp_greater_equal(position_accessor_idx->get(), model.accessors.size()))
			return util::Error("Primitive POSITION accessor index out of bounds");

		return extract_from_accessor<glm::vec3>(model, model.accessors[*position_accessor_idx]);
	}

	std::expected<std::optional<std::vector<glm::vec3>>, util::Error> get_primitive_normal_raw(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive
	) noexcept
	{
		const auto normal_accessor_idx = util::find_map(primitive.attributes, "NORMAL");

		if (!normal_accessor_idx) return std::nullopt;
		if (normal_accessor_idx->get() < 0) return util::Error("Primitive NORMAL has no accessor");
		if (std::cmp_greater_equal(normal_accessor_idx->get(), model.accessors.size()))
			return util::Error("Primitive NORMAL accessor index out of bounds");

		return extract_from_accessor<glm::vec3>(model, model.accessors[*normal_accessor_idx]);
	}

	std::expected<std::vector<glm::vec2>, util::Error> get_primitive_texcoord_raw(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive,
		const std::string& texcoord_name
	) noexcept
	{
		const auto texcoord_accessor_idx = util::find_map(primitive.attributes, texcoord_name);

		if (!texcoord_accessor_idx)
			return util::Error(std::format("Primitive has no {} attribute", texcoord_name));
		if (texcoord_accessor_idx->get() < 0)
			return util::Error("Primitive " + texcoord_name + " has no accessor");
		if (std::cmp_greater_equal(texcoord_accessor_idx->get(), model.accessors.size()))
			return util::Error("Primitive " + texcoord_name + " accessor index out of bounds");

		return extract_from_accessor<glm::vec2>(model, model.accessors[*texcoord_accessor_idx]);
	}

	static std::vector<glm::vec3> calc_normal(const std::vector<glm::vec3>& position_vertices) noexcept
	{
		std::vector<glm::vec3> normals;
		normals.reserve(position_vertices.size());

		for (const auto tri : position_vertices | std::views::chunk(3))
		{
			const auto normal = glm::normalize(glm::cross(tri[1] - tri[0], tri[2] - tri[0]));
			normals.push_back(normal);
			normals.push_back(normal);
			normals.push_back(normal);
		}

		return normals;
	}

	static glm::vec3 calc_tangent(
		const glm::vec3& pos0,
		const glm::vec3& pos1,
		const glm::vec3& pos2,
		const glm::vec2& uv0,
		const glm::vec2& uv1,
		const glm::vec2& uv2
	) noexcept
	{
		const glm::mat2x3 pos_delta(pos1 - pos0, pos2 - pos0);
		const glm::mat2 uv_delta(uv1 - uv0, uv2 - uv0);
		const glm::mat2 uv_delta_inv = glm::inverse(uv_delta);
		const glm::mat2x3 tangent_mat = pos_delta * uv_delta_inv;
		return glm::normalize(tangent_mat[0]);
	}

	std::expected<std::optional<std::vector<uint32_t>>, util::Error> get_primitive_index(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive
	) noexcept
	{
		if (primitive.indices < 0) return std::nullopt;
		if (std::cmp_greater_equal(primitive.indices, model.accessors.size()))
			return util::Error("Primitive index accessor index out of bounds");

		const auto& accessor = model.accessors[primitive.indices];

		switch (accessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		{
			auto result = extract_from_accessor<uint32_t>(model, model.accessors[primitive.indices]);
			if (!result) return result.error().propagate("Extract uint32_t index data failed");
			return std::optional(std::move(*result));
		}
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		{
			auto result = extract_from_accessor<uint16_t>(model, model.accessors[primitive.indices]);
			if (!result) return result.error().propagate("Extract uint16_t index data failed");
			return std::optional(std::vector<uint32_t>(std::from_range, *result));
		}
		default:
			return util::Error(
				std::format(
					"Unsupported index accessor component type {}",
					model.accessors[primitive.indices].componentType
				)
			);
		}
	}

	std::expected<std::vector<glm::vec3>, util::Error> get_position(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive,
		const std::optional<std::vector<uint32_t>>& index
	) noexcept
	{
		auto position_raw_result = get_primitive_position_raw(model, primitive);
		if (!position_raw_result)
			return position_raw_result.error().propagate("Get primitive POSITION data failed");

		auto remapped_position_vertices = unpack_from_indices(*position_raw_result, index);
		if (!remapped_position_vertices)
			return remapped_position_vertices.error().propagate("Unpack POSITION from indices failed");

		auto position_vertices_result = rearrange_vertices(*remapped_position_vertices, primitive.mode);
		if (!position_vertices_result)
			return position_vertices_result.error().propagate("Rearrange triangle POSITION data failed");

		return std::move(*position_vertices_result);
	}

	std::expected<std::vector<glm::vec3>, util::Error> get_normal(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive,
		const std::optional<std::vector<uint32_t>>& index,
		const std::vector<glm::vec3>& position_vertices
	) noexcept
	{
		auto normal_raw_result = get_primitive_normal_raw(model, primitive);
		if (!normal_raw_result)
			return normal_raw_result.error().propagate("Get primitive NORMAL data failed");

		auto normal_raw = std::move(*normal_raw_result);
		if (!normal_raw.has_value()) return calc_normal(position_vertices);

		auto remapped_normal_vertices = unpack_from_indices(*normal_raw, index);
		if (!remapped_normal_vertices)
			return remapped_normal_vertices.error().propagate("Unpack NORMAL from indices failed");

		auto normal_vertices = rearrange_vertices(*remapped_normal_vertices, primitive.mode);

		if (!normal_vertices)
			return normal_vertices.error().propagate("Rearrange triangle NORMAL data failed");
		if (normal_vertices->size() != position_vertices.size())
			return util::Error("NORMAL vertex count does not match POSITION vertex count");

		for (auto& normal : *normal_vertices) normal = glm::normalize(normal);

		return std::move(*normal_vertices);
	}

	std::expected<std::vector<glm::vec2>, util::Error> get_texcoord(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive,
		const std::optional<std::vector<uint32_t>>& index,
		const std::string& texcoord_name
	) noexcept
	{
		auto texcoord_raw_result = get_primitive_texcoord_raw(model, primitive, texcoord_name);
		if (!texcoord_raw_result)
			return texcoord_raw_result.error().propagate("Get primitive " + texcoord_name + " data failed");

		auto remapped_texcoord_vertices = unpack_from_indices(*texcoord_raw_result, index);
		if (!remapped_texcoord_vertices)
			return remapped_texcoord_vertices.error().propagate(
				std::format("Unpack {} from indices failed", texcoord_name)
			);

		auto texcoord_vertices_result = rearrange_vertices(*remapped_texcoord_vertices, primitive.mode);
		if (!texcoord_vertices_result)
			return texcoord_vertices_result.error().propagate(
				"Rearrange triangle " + texcoord_name + " data failed"
			);

		return std::move(*texcoord_vertices_result);
	}

	std::vector<glm::vec3> get_tangent(
		const std::vector<glm::vec3>& position_vertices,
		const std::vector<glm::vec2>& texcoord0_vertices
	) noexcept
	{
		std::vector<glm::vec3> tangents;
		tangents.reserve(position_vertices.size());

		for (const auto tri : std::views::zip(position_vertices, texcoord0_vertices) | std::views::chunk(3))
		{
			const auto tangent = calc_tangent(
				std::get<0>(tri[0]),
				std::get<0>(tri[1]),
				std::get<0>(tri[2]),
				std::get<1>(tri[0]),
				std::get<1>(tri[1]),
				std::get<1>(tri[2])
			);

			tangents.push_back(tangent);
			tangents.push_back(tangent);
			tangents.push_back(tangent);
		}

		return tangents;
	}
}