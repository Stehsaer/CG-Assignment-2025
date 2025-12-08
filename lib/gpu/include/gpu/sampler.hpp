#pragma once

#include <SDL3/SDL_gpu.h>
#include <expected>
#include <optional>

#include "resource-box.hpp"

namespace gpu
{
	class Sampler : public Resource_box<SDL_GPUSampler>
	{
	  public:

		Sampler(const Sampler&) = delete;
		Sampler& operator=(const Sampler&) = delete;
		Sampler(Sampler&&) = default;
		Sampler& operator=(Sampler&&) = default;
		~Sampler() noexcept = default;

		enum class Mipmap_mode
		{
			Nearest = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
			Linear = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR
		};

		enum class Address_mode
		{
			Repeat = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
			Mirrored_repeat = SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT,
			Clamp_to_edge = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
		};

		enum class Filter
		{
			Nearest = SDL_GPU_FILTER_NEAREST,
			Linear = SDL_GPU_FILTER_LINEAR
		};

		enum class Compare_op
		{
			Never = SDL_GPU_COMPAREOP_NEVER,
			Invalid = SDL_GPU_COMPAREOP_INVALID,
			Less = SDL_GPU_COMPAREOP_LESS,
			Equal = SDL_GPU_COMPAREOP_EQUAL,
			Less_or_equal = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
			Greater = SDL_GPU_COMPAREOP_GREATER,
			Not_equal = SDL_GPU_COMPAREOP_NOT_EQUAL,
			Greater_or_equal = SDL_GPU_COMPAREOP_GREATER_OR_EQUAL,
			Always = SDL_GPU_COMPAREOP_ALWAYS
		};

		struct Create_info
		{
			Filter min_filter = Filter::Linear;
			Filter mag_filter = Filter::Linear;
			Mipmap_mode mipmap_mode = Mipmap_mode::Linear;
			Address_mode address_mode_u = Address_mode::Repeat;
			Address_mode address_mode_v = Address_mode::Repeat;
			Address_mode address_mode_w = Address_mode::Repeat;
			float min_lod = 0.0f;
			float max_lod = 16.0f;
			float mip_lod_bias = 0.0f;
			std::optional<float> max_anisotropy = std::nullopt;
			std::optional<Compare_op> compare_op = std::nullopt;

			SDL_GPUSamplerCreateInfo create() const noexcept;
		};

		///
		/// @brief Creates a sampler object
		///
		/// @param create_info Sampler create info
		/// @return Sampler object, or error if failed
		///
		static std::expected<Sampler, util::Error> create(
			SDL_GPUDevice* device,
			const Create_info& create_info
		) noexcept;

	  private:

		using Resource_box<SDL_GPUSampler>::Resource_box;
	};
}