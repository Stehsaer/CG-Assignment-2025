#include "gltf/detail/mesh/raw-primitive-list.hpp"
#include "gltf/detail/mesh/data.hpp"

#include <ranges>

namespace gltf::detail::mesh
{
	std::expected<std::vector<Vertex>, util::Error> get_primitive_list(
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive
	) noexcept
	{
		/* Check Primitive Type */

		if (primitive.mode != TINYGLTF_MODE_TRIANGLES
			&& primitive.mode != TINYGLTF_MODE_TRIANGLE_FAN
			&& primitive.mode != TINYGLTF_MODE_TRIANGLE_STRIP)
			return util::Error("Only triangle primitives are supported");

		/* Get Index Data */

		auto index_raw_result = get_primitive_index(model, primitive);
		if (!index_raw_result) return index_raw_result.error().propagate("Get primitive index data failed");
		const auto index = std::move(*index_raw_result);

		/* Get Position Data */

		auto position_result = get_position(model, primitive, index);
		if (!position_result) return position_result.error().propagate("Get primitive POSITION failed");
		const auto position_vertices = std::move(*position_result);

		/* Get Normal Data */

		auto normal_result = get_normal(model, primitive, index, position_vertices);
		if (!normal_result) return normal_result.error().propagate("Get primitive NORMAL failed");
		const auto normal_vertices = std::move(*normal_result);

		/* Get Texcoord0 Data */

		auto texcoord0_result = get_texcoord(model, primitive, index, "TEXCOORD_0");
		if (!texcoord0_result) return texcoord0_result.error().propagate("Get primitive TEXCOORD_0 failed");
		const auto texcoord0_vertices = std::move(*texcoord0_result);

		/* Get Tangent data */

		const auto tangent_vertices = get_tangent(position_vertices, texcoord0_vertices);

		/* Check Size */

		if (position_vertices.size() != normal_vertices.size()
			|| position_vertices.size() != texcoord0_vertices.size()
			|| position_vertices.size() != tangent_vertices.size())
			return util::Error("Primitive attribute vertex counts do not match");

		/* Assemble Primitive */

		auto primitives =
			std::views::zip(position_vertices, normal_vertices, texcoord0_vertices, tangent_vertices)
			| std::views::transform([](const auto& tup) {
				  auto vertex = Vertex{
					  .position = std::get<0>(tup),
					  .normal = std::get<1>(tup),
					  .tangent = std::get<3>(tup),
					  .texcoord = std::get<2>(tup),
				  };

				  const auto bitangent = glm::cross(vertex.tangent, vertex.normal);
				  const auto tangent = glm::cross(vertex.normal, bitangent);
				  vertex.tangent = tangent;

				  return vertex;
			  })
			| std::ranges::to<std::vector>();

		return primitives;
	}
}