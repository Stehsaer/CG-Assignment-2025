#include "gpu/copy-pass.hpp"

namespace gpu
{
	void CopyPass::copy_buffer_to_buffer(
		const Buffer& src_buffer,
		uint32_t src_offset,
		const Buffer& dst_buffer,
		uint32_t dst_offset,
		uint32_t size,
		bool cycle
	) const noexcept
	{
		assert(resource != nullptr);

		const SDL_GPUBufferLocation src_location = {.buffer = src_buffer, .offset = src_offset};
		const SDL_GPUBufferLocation dst_location = {.buffer = dst_buffer, .offset = dst_offset};
		SDL_CopyGPUBufferToBuffer(resource, &src_location, &dst_location, size, cycle);
	}

	void CopyPass::copy_texture_to_texture(
		const SDL_GPUTextureLocation& src_location,
		const SDL_GPUTextureLocation& dst_location,
		uint32_t region_width,
		uint32_t region_height,
		uint32_t region_depth,
		bool cycle
	) const noexcept
	{
		assert(resource != nullptr);

		SDL_CopyGPUTextureToTexture(
			resource,
			&src_location,
			&dst_location,
			region_width,
			region_height,
			region_depth,
			cycle
		);
	}

	void CopyPass::upload_to_buffer(
		const TransferBuffer& src_buffer,
		uint32_t src_offset,
		const Buffer& dst_buffer,
		uint32_t dst_offset,
		uint32_t size,
		bool cycle
	) const noexcept
	{
		assert(resource != nullptr);

		const SDL_GPUTransferBufferLocation src_location =
			{.transfer_buffer = src_buffer, .offset = src_offset};
		const SDL_GPUBufferRegion dst_region = {.buffer = dst_buffer, .offset = dst_offset, .size = size};
		SDL_UploadToGPUBuffer(resource, &src_location, &dst_region, cycle);
	}

	void CopyPass::upload_to_texture(
		const SDL_GPUTextureTransferInfo& src_info,
		const SDL_GPUTextureRegion& dst_region,
		bool cycle
	) const noexcept
	{
		assert(resource != nullptr);
		SDL_UploadToGPUTexture(resource, &src_info, &dst_region, cycle);
	}

	void CopyPass::download_from_buffer(
		const Buffer& src_buffer,
		uint32_t src_offset,
		const TransferBuffer& dst_buffer,
		uint32_t dst_offset,
		uint32_t size
	) const noexcept
	{
		assert(resource != nullptr);

		const SDL_GPUBufferRegion src_region = {.buffer = src_buffer, .offset = src_offset, .size = size};
		const SDL_GPUTransferBufferLocation dst_location =
			{.transfer_buffer = dst_buffer, .offset = dst_offset};
		SDL_DownloadFromGPUBuffer(resource, &src_region, &dst_location);
	}

	void CopyPass::download_from_texture(
		const SDL_GPUTextureRegion& src_region,
		const SDL_GPUTextureTransferInfo& dst_info
	) const noexcept
	{
		assert(resource != nullptr);
		SDL_DownloadFromGPUTexture(resource, &src_region, &dst_info);
	}
}