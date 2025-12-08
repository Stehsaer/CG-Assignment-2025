#pragma once

#include <gpu.hpp>
#include <image/repr.hpp>
#include <util/as-byte.hpp>

namespace graphic
{
	///
	/// @brief Creates a command buffer, executes the copy task then submits it
	///
	/// @param task Copy task
	///
	std::expected<void, util::Error> execute_copy_task(
		SDL_GPUDevice* device,
		const std::function<void(const gpu::Copy_pass&)>& task
	) noexcept;

	///
	/// @brief Create a buffer and uploads input binary data to it. No cycling is performed.
	/// @details
	/// - This function is designed for initializing buffers with data at **loading stage**.
	/// - It has some overhead, which should be acceptable at loading stage but not at render-time.
	/// - Don't use it on-the-fly during rendering. Manually copy on a copy pass from the main command buffer.
	///
	/// @param usage Buffer usage
	/// @param data Binary data
	/// @return Created buffer, or error
	///
	std::expected<gpu::Buffer, util::Error> create_buffer_with_data(
		SDL_GPUDevice* device,
		gpu::Buffer::Usage usage,
		std::span<const std::byte> data
	) noexcept;

	template <typename T>
	std::expected<gpu::Texture, util::Error> create_texture_from_image(
		SDL_GPUDevice* device,
		gpu::Texture::Format format,
		const image::Image_container<T>& image
	) noexcept
	{
		auto texture = gpu::Texture::create(device, format.create(image.size.x, image.size.y, 1, 1));
		if (!texture) return texture.error().propagate("Create texture failed");

		auto transfer_buffer = gpu::Transfer_buffer::create_from_data(device, util::as_bytes(image.pixels));
		if (!transfer_buffer) return transfer_buffer.error().propagate("Create transfer buffer failed");

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
			graphic::execute_copy_task(device, [&transfer_info, &texture_region](const auto& copy_pass) {
				copy_pass.upload_to_texture(transfer_info, texture_region, false);
			});
		if (!copy_task_result) return copy_task_result.error().propagate("Execute copy task failed");

		return texture;
	}

	template <typename T>
	std::expected<gpu::Texture, util::Error> create_texture_from_mipmap_chain(
		SDL_GPUDevice* device,
		gpu::Texture::Format format,
		const std::vector<image::Image_container<T>>& mipmap_chain
	) noexcept
	{
		auto texture = gpu::Texture::create(
			device,
			format.create(mipmap_chain[0].size.x, mipmap_chain[0].size.y, 1, mipmap_chain.size())
		);
		if (!texture) return texture.error().propagate("Create texture failed");

		const auto transfer_buffers =
			mipmap_chain
			| std::views::transform([&](const auto& image) {
				  return gpu::Transfer_buffer::create_from_data(device, util::as_bytes(image.pixels));
			  })
			| std::ranges::to<std::vector>();

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

		const auto copy_task_result = graphic::execute_copy_task(device, [&](const auto& copy_pass) {
			for (const auto [transfer_buffer, info, region] :
				 std::views::zip(transfer_buffers, transfer_infos, transfer_regions))
			{
				copy_pass.upload_to_texture(info, region, false);
			}
		});
		if (!copy_task_result) return copy_task_result.error().propagate("Execute copy task failed");

		return texture;
	}
}