#include "gltf/accessor.hpp"

#include <glm/gtc/quaternion.hpp>

namespace gltf::detail
{
	template <>
	bool check_accessor_for_type<uint32_t>(const tinygltf::Accessor& accessor) noexcept
	{
		return accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT
			&& accessor.type == TINYGLTF_TYPE_SCALAR;
	}

	template <>
	bool check_accessor_for_type<uint16_t>(const tinygltf::Accessor& accessor) noexcept
	{
		return accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT
			&& accessor.type == TINYGLTF_TYPE_SCALAR;
	}

	template <>
	bool check_accessor_for_type<float>(const tinygltf::Accessor& accessor) noexcept
	{
		return accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT
			&& accessor.type == TINYGLTF_TYPE_SCALAR;
	}

	template <>
	bool check_accessor_for_type<glm::vec2>(const tinygltf::Accessor& accessor) noexcept
	{
		return accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC2;
	}

	template <>
	bool check_accessor_for_type<glm::vec3>(const tinygltf::Accessor& accessor) noexcept
	{
		return accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC3;
	}

	template <>
	bool check_accessor_for_type<glm::vec4>(const tinygltf::Accessor& accessor) noexcept
	{
		return accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC4;
	}

	template <>
	bool check_accessor_for_type<glm::quat>(const tinygltf::Accessor& accessor) noexcept
	{
		return accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC4;
	}
}