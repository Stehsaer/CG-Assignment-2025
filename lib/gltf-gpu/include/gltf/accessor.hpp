#pragma once

#include <algorithm>
#include <glm/glm.hpp>
#include <ranges>
#include <tiny_gltf.h>
#include <util/error.hpp>
#include <vector>

namespace gltf
{
	namespace detail
	{
		///
		/// @brief Check if the accessor matches the requested type.
		///
		/// @tparam T Requested type
		/// @param accessor Target accessor
		/// @return True if matches, false otherwise
		///
		template <typename T>
		bool check_accessor_for_type(const tinygltf::Accessor& accessor) noexcept;
	}

	///
	/// @brief Extract raw typed data from an accessor.
	///
	/// @tparam T Type to extract
	/// @param model Tinygltf model
	/// @param accessor Accessor
	/// @return Raw typed data in `std::vector`, or error on failure
	///
	template <typename T>
	std::expected<std::vector<T>, util::Error> extract_raw_from_accessor(
		const tinygltf::Model& model,
		const tinygltf::Accessor& accessor
	) noexcept
	{
		/* Accessor Check */

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

		return data;
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
	std::expected<std::vector<T>, util::Error> extract_from_accessor(
		const tinygltf::Model& model,
		const tinygltf::Accessor& accessor
	) noexcept
	{
		if (!detail::check_accessor_for_type<T>(accessor))
			return util::Error(std::format("Accessor type ({}) doesn't match requested type", accessor.type));

		auto data = extract_raw_from_accessor<T>(model, accessor);
		if (!data) return data.error().propagate("Extract raw data failed");

		return std::move(*data);
	}

	/* Template Instantiation */

	namespace detail
	{
		template <>
		bool check_accessor_for_type<uint16_t>(const tinygltf::Accessor& accessor) noexcept;

		template <>
		bool check_accessor_for_type<uint32_t>(const tinygltf::Accessor& accessor) noexcept;

		template <>
		bool check_accessor_for_type<float>(const tinygltf::Accessor& accessor) noexcept;

		template <>
		bool check_accessor_for_type<glm::vec2>(const tinygltf::Accessor& accessor) noexcept;

		template <>
		bool check_accessor_for_type<glm::vec3>(const tinygltf::Accessor& accessor) noexcept;

		template <>
		bool check_accessor_for_type<glm::vec4>(const tinygltf::Accessor& accessor) noexcept;

		template <>
		bool check_accessor_for_type<glm::quat>(const tinygltf::Accessor& accessor) noexcept;
	}
}
