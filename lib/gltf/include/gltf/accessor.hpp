///
/// @file accessor.hpp
/// @brief Provides a function to extract typed data from a glTF accessor
///

#pragma once

#include "util/error.hpp"
#include <algorithm>
#include <glm/glm.hpp>
#include <ranges>
#include <tiny_gltf.h>
#include <vector>

namespace gltf
{
	namespace detail
	{
		template <typename T>
		class AccessTypeTrait
		{
		  public:

			static constexpr bool available = false;
		};

		template <>
		class AccessTypeTrait<uint8_t>
		{
		  public:

			static constexpr bool available = true;
			static constexpr int component_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
			static constexpr int type = TINYGLTF_TYPE_SCALAR;
		};

		template <>
		class AccessTypeTrait<uint16_t>
		{
		  public:

			static constexpr bool available = true;
			static constexpr int component_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
			static constexpr int type = TINYGLTF_TYPE_SCALAR;
		};

		template <>
		class AccessTypeTrait<uint32_t>
		{
		  public:

			static constexpr bool available = true;
			static constexpr int component_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
			static constexpr int type = TINYGLTF_TYPE_SCALAR;
		};

		template <>
		class AccessTypeTrait<float>
		{
		  public:

			static constexpr bool available = true;
			static constexpr int component_type = TINYGLTF_COMPONENT_TYPE_FLOAT;
			static constexpr int type = TINYGLTF_TYPE_SCALAR;
		};

		template <>
		class AccessTypeTrait<glm::vec2>
		{
		  public:

			static constexpr bool available = true;
			static constexpr int component_type = TINYGLTF_COMPONENT_TYPE_FLOAT;
			static constexpr int type = TINYGLTF_TYPE_VEC2;
		};

		template <>
		class AccessTypeTrait<glm::vec3>
		{
		  public:

			static constexpr bool available = true;
			static constexpr int component_type = TINYGLTF_COMPONENT_TYPE_FLOAT;
			static constexpr int type = TINYGLTF_TYPE_VEC3;
		};

		template <>
		class AccessTypeTrait<glm::vec4>
		{
		  public:

			static constexpr bool available = true;
			static constexpr int component_type = TINYGLTF_COMPONENT_TYPE_FLOAT;
			static constexpr int type = TINYGLTF_TYPE_VEC4;
		};

		template <>
		class AccessTypeTrait<glm::quat>
		{
		  public:

			static constexpr bool available = true;
			static constexpr int component_type = TINYGLTF_COMPONENT_TYPE_FLOAT;
			static constexpr int type = TINYGLTF_TYPE_VEC4;
		};

		template <>
		class AccessTypeTrait<glm::u32vec4>
		{
		  public:

			static constexpr bool available = true;
			static constexpr int component_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
			static constexpr int type = TINYGLTF_TYPE_VEC4;
		};

		template <>
		class AccessTypeTrait<glm::u16vec4>
		{
		  public:

			static constexpr bool available = true;
			static constexpr int component_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
			static constexpr int type = TINYGLTF_TYPE_VEC4;
		};

		template <>
		class AccessTypeTrait<glm::u8vec4>
		{
		  public:

			static constexpr bool available = true;
			static constexpr int component_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
			static constexpr int type = TINYGLTF_TYPE_VEC4;
		};

		template <>
		class AccessTypeTrait<glm::mat4>
		{
		  public:

			static constexpr bool available = true;
			static constexpr int component_type = TINYGLTF_COMPONENT_TYPE_FLOAT;
			static constexpr int type = TINYGLTF_TYPE_MAT4;
		};

		template <typename T>
		bool check_accessor_for_type(const tinygltf::Accessor& accessor) noexcept
		{
			return accessor.componentType == AccessTypeTrait<T>::component_type
				&& accessor.type == AccessTypeTrait<T>::type;
		}
	}

	///
	/// @brief Extract typed data from an accessor
	///
	/// @tparam T Type to extract
	/// @param model Tinygltf model
	/// @param accessor Accessor
	/// @return Extract result, or error on failure
	///
	template <typename T>
		requires(detail::AccessTypeTrait<T>::available)
	std::expected<std::vector<T>, util::Error> extract_from_accessor(
		const tinygltf::Model& model,
		const tinygltf::Accessor& accessor
	) noexcept
	{
		/* Accessor Check */

		if (!detail::check_accessor_for_type<T>(accessor))
			return util::Error(std::format("Accessor type ({}) doesn't match requested type", accessor.type));

		if (accessor.bufferView < 0) return util::Error("Accessor has no buffer view");
		if (std::cmp_greater_equal(accessor.bufferView, static_cast<int>(model.bufferViews.size())))
			return util::Error("Accessor buffer view index out of bounds");

		const auto& buffer_view = model.bufferViews[accessor.bufferView];

		/* BufferView Check */

		if (buffer_view.buffer < 0) return util::Error("BufferView has no buffer");
		if (std::cmp_greater_equal(buffer_view.buffer, static_cast<int>(model.buffers.size())))
			return util::Error("BufferView buffer index out of bounds");

		const auto& buffer = model.buffers[buffer_view.buffer];

		/* Buffer Check */

		if (buffer.data.empty()) return util::Error("Buffer has no data");
		if (buffer_view.byteOffset + buffer_view.byteLength > buffer.data.size())
			return util::Error("BufferView byte range out of bounds of buffer data");

		/* Size Calculation */

		const auto elem_count = accessor.count;
		const auto elem_size = sizeof(T);
		const auto byte_stride = buffer_view.byteStride == 0 ? elem_size : buffer_view.byteStride;
		const auto byte_offset = buffer_view.byteOffset + accessor.byteOffset;

		if (byte_offset + elem_count * byte_stride > buffer.data.size())
			return util::Error("Accessor byte range out of bounds of buffer data");
		if (elem_size > byte_stride) return util::Error("Accessor element size greater than byte stride");

		/* Extraction */

		std::vector<T> data(elem_count);
		for (const auto idx : std::views::iota(0u, elem_count))
			std::memcpy(&data[idx], &buffer.data[byte_offset + idx * byte_stride], elem_size);

		return std::move(data);
	}

	/* Template Instantiation */
}
