#include "graphics/util/quick-create.hpp"
#include "graphics/util/quick-copy.hpp"

#include <ranges>

namespace graphics
{
	std::expected<gpu::Buffer, util::Error> create_buffer_from_data(
		SDL_GPUDevice* device,
		gpu::Buffer::Usage usage,
		std::span<const std::byte> data,
		const std::string& name
	) noexcept
	{
		auto buffer = gpu::Buffer::create(device, usage, data.size(), name);
		if (!buffer) return buffer.error().forward("Create buffer failed");

		auto transfer_buffer = gpu::TransferBuffer::create_from_data(device, data);
		if (!transfer_buffer) return transfer_buffer.error().forward("Create transfer buffer failed");

		const auto copy_result = execute_copy_task(device, [&](const gpu::CopyPass& copy_pass) {
			copy_pass.upload_to_buffer(*transfer_buffer, 0, *buffer, 0, data.size(), false);
		});

		if (!copy_result) return copy_result.error().forward("Execute copy task failed");

		return buffer;
	}

	std::expected<gpu::Texture, util::Error> detail::create_texture_from_image_internal(
		SDL_GPUDevice* device,
		gpu::Texture::Format format,
		ImageData image,
		const std::string& name
	) noexcept
	{
		if (!format.supported_on(device)) return util::Error("Texture format not supported on device");

		auto texture = gpu::Texture::create(device, format.create(image.size.x, image.size.y, 1, 1), name);
		if (!texture) return texture.error().forward("Create texture failed");

		auto transfer_buffer = gpu::TransferBuffer::create_from_data(device, image.pixels);
		if (!transfer_buffer) return transfer_buffer.error().forward("Create transfer buffer failed");

		const SDL_GPUTextureTransferInfo transfer_info{
			.transfer_buffer = *transfer_buffer,
			.offset = 0,
			.pixels_per_row = image.size.x,
			.rows_per_layer = image.size.y
		};

		const SDL_GPUTextureRegion texture_region{
			.texture = *texture,
			.mip_level = 0,
			.layer = 0,
			.x = 0,
			.y = 0,
			.z = 0,
			.w = uint32_t(image.size.x),
			.h = uint32_t(image.size.y),
			.d = 1
		};

		const auto copy_task_result =
			execute_copy_task(device, [&transfer_info, &texture_region](const auto& copy_pass) {
				copy_pass.upload_to_texture(transfer_info, texture_region, false);
			});
		if (!copy_task_result) return copy_task_result.error().forward("Execute copy task failed");

		return texture;
	}

	std::expected<gpu::Texture, util::Error> detail::create_texture_from_mipmap_internal(
		SDL_GPUDevice* device,
		gpu::Texture::Format format,
		std::span<const ImageData> mipmap_chain,
		const std::string& name
	) noexcept
	{
		if (!format.supported_on(device)) return util::Error("Texture format not supported on device");

		auto texture = gpu::Texture::create(
			device,
			format.create(mipmap_chain[0].size.x, mipmap_chain[0].size.y, 1, mipmap_chain.size()),
			name
		);
		if (!texture) return texture.error().forward("Create texture failed");

		const auto transfer_buffers =
			mipmap_chain
			| std::views::transform([&](const ImageData& image) {
				  return gpu::TransferBuffer::create_from_data(device, image.pixels);
			  })
			| std::ranges::to<std::vector>();
		for (const auto& buffer : transfer_buffers)
			if (!buffer) return buffer.error().forward("Create transfer buffer failed");

		const auto transfer_infos =
			mipmap_chain
			| std::views::enumerate
			| std::views::transform([&](const auto& indexed_image) {
				  const auto& [mip_level, image] = indexed_image;
				  return SDL_GPUTextureTransferInfo{
					  .transfer_buffer = *transfer_buffers[mip_level],
					  .offset = 0,
					  .pixels_per_row = image.size.x,
					  .rows_per_layer = image.size.y
				  };
			  })
			| std::ranges::to<std::vector>();

		const auto transfer_regions =
			mipmap_chain
			| std::views::enumerate
			| std::views::transform([&](const auto& indexed_image) {
				  const auto& [mip_level, image] = indexed_image;
				  return SDL_GPUTextureRegion{
					  .texture = *texture,
					  .mip_level = uint32_t(mip_level),
					  .layer = 0,
					  .x = 0,
					  .y = 0,
					  .z = 0,
					  .w = uint32_t(image.size.x),
					  .h = uint32_t(image.size.y),
					  .d = 1
				  };
			  })
			| std::ranges::to<std::vector>();

		const auto copy_task_result = execute_copy_task(device, [&](const auto& copy_pass) {
			for (const auto [transfer_buffer, info, region] :
				 std::views::zip(transfer_buffers, transfer_infos, transfer_regions))
			{
				copy_pass.upload_to_texture(info, region, false);
			}
		});
		if (!copy_task_result) return copy_task_result.error().forward("Execute copy task failed");

		return texture;
	}
}