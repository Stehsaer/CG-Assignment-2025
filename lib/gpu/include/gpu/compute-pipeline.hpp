#pragma once

#include <SDL3/SDL_gpu.h>
#include <expected>
#include <span>

#include "resource-box.hpp"
#include "util/error.hpp"

namespace gpu
{
	///
	/// @brief Compute pipeline
	///
	///
	class ComputePipeline : public ResourceBox<SDL_GPUComputePipeline>
	{
	  public:

		ComputePipeline(const ComputePipeline&) = delete;
		ComputePipeline& operator=(const ComputePipeline&) = delete;
		ComputePipeline(ComputePipeline&&) noexcept = default;
		ComputePipeline& operator=(ComputePipeline&&) noexcept = default;
		~ComputePipeline() noexcept = default;

		struct CreateInfo
		{
			std::span<const std::byte> shader_data;
			uint32_t num_samplers = 0;
			uint32_t num_readonly_storage_textures = 0;
			uint32_t num_readwrite_storage_textures = 0;
			uint32_t num_readonly_storage_buffers = 0;
			uint32_t num_readwrite_storage_buffers = 0;
			uint32_t num_uniform_buffers = 0;
			uint32_t threadcount_x;
			uint32_t threadcount_y;
			uint32_t threadcount_z;
		};

		///
		/// @brief Create a compute pipeline
		///
		/// @return Compute pipeline object, or error if failed
		///
		static std::expected<ComputePipeline, util::Error> create(
			SDL_GPUDevice* device,
			const CreateInfo& create_info,
			const std::string& name
		) noexcept;

	  private:

		using ResourceBox<SDL_GPUComputePipeline>::ResourceBox;
	};
}