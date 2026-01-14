#include "image/io.hpp"

namespace image::internal
{
	template <>
	LoadResult<Precision_t<Precision::U8>> load_from_memory<Precision::U8>(
		std::span<const std::byte> data,
		int desired_channel
	) noexcept
	{
		LoadResult<Precision_t<Precision::U8>> result;
		result.pixels = stbi_load_from_memory(
			reinterpret_cast<const stbi_uc*>(data.data()),
			static_cast<int>(data.size()),
			&result.width,
			&result.height,
			&result.channels,
			desired_channel
		);
		return result;
	}

	template <>
	LoadResult<Precision_t<Precision::U16>> load_from_memory<Precision::U16>(
		std::span<const std::byte> data,
		int desired_channel
	) noexcept
	{
		LoadResult<Precision_t<Precision::U16>> result;
		result.pixels = stbi_load_16_from_memory(
			reinterpret_cast<const stbi_uc*>(data.data()),
			static_cast<int>(data.size()),
			&result.width,
			&result.height,
			&result.channels,
			desired_channel
		);
		return result;
	}

	template <>
	LoadResult<Precision_t<Precision::F32>> load_from_memory<Precision::F32>(
		std::span<const std::byte> data,
		int desired_channel
	) noexcept
	{
		LoadResult<Precision_t<Precision::F32>> result;
		result.pixels = stbi_loadf_from_memory(
			reinterpret_cast<const stbi_uc*>(data.data()),
			static_cast<int>(data.size()),
			&result.width,
			&result.height,
			&result.channels,
			desired_channel
		);
		return result;
	}
}