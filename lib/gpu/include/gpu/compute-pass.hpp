#pragma once

#include "buffer.hpp"
#include "compute-pipeline.hpp"
#include "scoped-pass.hpp"

#include <SDL3/SDL_gpu.h>
#include <span>

namespace gpu
{
	///
	/// @brief Compute pass
	///
	///
	class ComputePass : public ScopedPass<SDL_GPUComputePass>
	{
	  public:

		///
		/// @brief Bind a compute pipeline
		///
		/// @param pipeline Compute pipeline
		///
		void bind_pipeline(const ComputePipeline& pipeline) const noexcept;

		///
		/// @brief Bind samplers
		///
		/// @param first_slot First binding slot
		/// @param samplers List of samplers
		///
		void bind_samplers(
			uint32_t first_slot,
			std::span<const SDL_GPUTextureSamplerBinding> samplers
		) const noexcept;

		///
		/// @brief Bind some samplers
		///
		/// @param first_slot First binding slot
		/// @param samplers Sampler packs, the first one is bound to first_slot, and so on
		///
		template <std::convertible_to<SDL_GPUTextureSamplerBinding>... Args>
			requires(sizeof...(Args) > 0)
		void bind_samplers(uint32_t first_slot, Args&&... samplers) const noexcept
		{
			const std::array<SDL_GPUTextureSamplerBinding, sizeof...(Args)> sampler_arr = {
				std::forward<Args>(samplers)...
			};
			bind_samplers(first_slot, sampler_arr);
		}

		///
		/// @brief Bind storage textures
		///
		/// @param first_slot First binding slot
		/// @param textures List of textures
		///
		void bind_storage_textures(
			uint32_t first_slot,
			std::span<SDL_GPUTexture* const> textures
		) const noexcept;

		///
		/// @brief Bind some storage textures
		///
		/// @param first_slot First binding slot
		/// @param textures Texture packs, the first one is bound to first_slot, and so on
		///
		template <std::convertible_to<SDL_GPUTexture*>... Args>
			requires(sizeof...(Args) > 0)
		void bind_storage_textures(uint32_t first_slot, Args&&... textures) const noexcept
		{
			const std::array<SDL_GPUTexture*, sizeof...(Args)> texture_arr = {
				std::forward<Args>(textures)...
			};
			bind_storage_textures(first_slot, texture_arr);
		}

		///
		/// @brief Bind storage buffers
		///
		/// @param first_slot First binding slot
		/// @param buffers List of buffers
		///
		void bind_storage_buffers(
			uint32_t first_slot,
			std::span<SDL_GPUBuffer* const> buffers
		) const noexcept;

		///
		/// @brief Bind some storage buffers
		///
		/// @param first_slot First binding slot
		/// @param buffers Buffer packs, the first one is bound to first_slot, and so on
		///
		template <std::convertible_to<SDL_GPUBuffer* const>... Args>
			requires(sizeof...(Args) > 0)
		void bind_storage_buffers(uint32_t first_slot, Args&&... buffers) const noexcept
		{
			const std::array<SDL_GPUBuffer*, sizeof...(Args)> buffer_arr = {std::forward<Args>(buffers)...};
			bind_storage_buffers(first_slot, buffer_arr);
		}

		///
		/// @brief Launch a compute task
		///
		/// @param group_count_x X-axis work group count
		/// @param group_count_y Y-axis work group count
		/// @param group_count_z Z-axis work group count
		///
		void dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z) const noexcept;

		///
		/// @brief Launch indirect compute task
		///
		/// @param buffer Indirect buffer
		/// @param offset Offset in the buffer in bytes
		///
		void dispatch_indirect(const Buffer& buffer, uint32_t offset) const noexcept;

	  private:

		using ScopedPass<SDL_GPUComputePass>::ScopedPass;
	};
}