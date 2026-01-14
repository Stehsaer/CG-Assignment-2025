#pragma once

#include "buffer.hpp"
#include "scoped-pass.hpp"

#include <SDL3/SDL_gpu.h>

namespace gpu
{
	class CopyPass : public ScopedPass<SDL_GPUCopyPass>
	{
	  public:

		///
		/// @brief Copies data between two buffers on the GPU side
		///
		/// @param src_buffer Source buffer
		/// @param src_offset Source buffer offset
		/// @param dst_buffer Destination buffer
		/// @param dst_offset Destination buffer offset
		/// @param size Number of bytes to copy
		/// @param cycle Use cycle mode
		///
		void copy_buffer_to_buffer(
			const Buffer& src_buffer,
			uint32_t src_offset,
			const Buffer& dst_buffer,
			uint32_t dst_offset,
			uint32_t size,
			bool cycle
		) const noexcept;

		///
		/// @brief Copies data between two textures on the GPU side
		///
		/// @param src_location Source texture location
		/// @param dst_location Destination texture location
		/// @param region_width Copy region width
		/// @param region_height Copy region height
		/// @param region_depth Copy region depth
		/// @param cycle Use cycle mode
		///
		void copy_texture_to_texture(
			const SDL_GPUTextureLocation& src_location,
			const SDL_GPUTextureLocation& dst_location,
			uint32_t region_width,
			uint32_t region_height,
			uint32_t region_depth,
			bool cycle
		) const noexcept;

		///
		/// @brief Uploads data from a transfer buffer on the CPU side to a buffer on the GPU side
		///
		/// @param src_buffer Source transfer buffer
		/// @param src_offset Source buffer offset
		/// @param dst_buffer Destination buffer
		/// @param dst_offset Destination buffer offset
		/// @param size Number of bytes to copy
		/// @param cycle Use cycle mode
		///
		void upload_to_buffer(
			const TransferBuffer& src_buffer,
			uint32_t src_offset,
			const Buffer& dst_buffer,
			uint32_t dst_offset,
			uint32_t size,
			bool cycle
		) const noexcept;

		///
		/// @brief Uploads data from a transfer buffer on the CPU side to a texture on the GPU side
		///
		/// @param src_info Transfer information (including source transfer buffer, offset, row pitch, etc.)
		/// @param dst_region Destination texture region
		/// @param cycle Use cycle mode
		///
		void upload_to_texture(
			const SDL_GPUTextureTransferInfo& src_info,
			const SDL_GPUTextureRegion& dst_region,
			bool cycle
		) const noexcept;

		///
		/// @brief Downloads data from a buffer on the GPU side to a transfer buffer on the CPU side
		///
		/// @param src_buffer Source buffer
		/// @param src_offset Source buffer offset
		/// @param dst_buffer Destination transfer buffer
		/// @param dst_offset Destination buffer offset
		/// @param size Number of bytes to copy
		///
		void download_from_buffer(
			const Buffer& src_buffer,
			uint32_t src_offset,
			const TransferBuffer& dst_buffer,
			uint32_t dst_offset,
			uint32_t size
		) const noexcept;

		///
		/// @brief Downloads data from a texture on the GPU side to a transfer buffer on the CPU side
		///
		/// @param src_region Source texture region
		/// @param dst_info Destination transfer information
		///
		void download_from_texture(
			const SDL_GPUTextureRegion& src_region,
			const SDL_GPUTextureTransferInfo& dst_info
		) const noexcept;

	  private:

		using ScopedPass<SDL_GPUCopyPass>::ScopedPass;
	};
}