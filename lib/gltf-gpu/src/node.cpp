#include "gltf/node.hpp"

#include <algorithm>
#include <ranges>

namespace gltf
{
	std::expected<Node, util::Error> Node::from_tinygltf(
		const tinygltf::Model& model,
		const tinygltf::Node& node
	) noexcept
	{
		Node result;

		if (std::ranges::any_of(node.children, [&](int child_index) {
				return child_index < 0 || std::cmp_greater_equal(child_index, model.nodes.size());
			}))
			return util::Error("Node has invalid child index");
		result.children = std::vector<uint32_t>(std::from_range, node.children);

		if (node.mesh >= 0)
		{
			if (std::cmp_greater_equal(static_cast<uint32_t>(node.mesh), model.meshes.size()))
				return util::Error(std::format("Node has invalid mesh index {}", node.mesh));
			result.mesh = node.mesh;
		}

		if (node.skin >= 0)
		{
			if (std::cmp_greater_equal(static_cast<uint32_t>(node.skin), model.skins.size()))
				return util::Error(std::format("Node has invalid skin index {}", node.skin));
			result.skin = node.skin;
		}

		if (node.matrix.size() == 16)
		{
			glm::mat4 matrix;
			std::ranges::copy(
				node.matrix | std::views::transform([](double v) { return static_cast<float>(v); }),
				&matrix[0][0]
			);
			result.transform = matrix;
		}
		else if (node.matrix.empty())
		{
			Transform transform;

			if (node.translation.size() == 3)
				transform.translation = glm::vec3{
					static_cast<float>(node.translation[0]),
					static_cast<float>(node.translation[1]),
					static_cast<float>(node.translation[2])
				};
			else if (!node.translation.empty())
				return util::Error("Node has invalid translation array size");

			if (node.rotation.size() == 4)
				transform.rotation = glm::quat{
					static_cast<float>(node.rotation[3]),
					static_cast<float>(node.rotation[0]),
					static_cast<float>(node.rotation[1]),
					static_cast<float>(node.rotation[2])
				};
			else if (!node.rotation.empty())
				return util::Error("Node has invalid rotation array size");

			if (node.scale.size() == 3)
				transform.scale = glm::vec3{
					static_cast<float>(node.scale[0]),
					static_cast<float>(node.scale[1]),
					static_cast<float>(node.scale[2])
				};
			else if (!node.scale.empty())
				return util::Error("Node has invalid scale array size");

			result.transform = transform;
		}

		return result;
	}
}