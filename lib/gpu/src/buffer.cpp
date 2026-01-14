#include "gpu/buffer.hpp"
#include "gpu/util.hpp"

namespace gpu
{
	std::expected<Buffer, util::Error> Buffer::create(
		SDL_GPUDevice* device,
		Usage usage,
		uint32_t size,
		const std::string& name
	) noexcept
	{
		assert(device != nullptr);

		if (size == 0) return util::Error("Buffer size must be greater than 0");

		const SDL_GPUBufferCreateInfo create_info{.usage = usage, .size = size, .props = 0};

		auto* const buffer = SDL_CreateGPUBuffer(device, &create_info);
		if (buffer == nullptr) RETURN_SDL_ERROR;

		SDL_SetGPUBufferName(device, buffer, name.c_str());

		return Buffer(device, buffer);
	}

	Buffer::Usage::operator SDL_GPUBufferUsageFlags(this Usage self) noexcept
	{
		return std::bit_cast<uint8_t>(self) & 0x3f;
	}

	std::expected<TransferBuffer, util::Error> TransferBuffer::create(
		SDL_GPUDevice* device,
		Usage usage,
		uint32_t size
	) noexcept
	{
		assert(device != nullptr);

		if (size == 0) return util::Error("Transfer buffer size must be greater than 0");

		const SDL_GPUTransferBufferCreateInfo
			create_info{.usage = static_cast<SDL_GPUTransferBufferUsage>(usage), .size = size, .props = 0};

		auto* const transfer_buffer = SDL_CreateGPUTransferBuffer(device, &create_info);
		if (transfer_buffer == nullptr) RETURN_SDL_ERROR;

		auto buffer = TransferBuffer(device, transfer_buffer);
		buffer.size = size;
		buffer.usage = usage;

		return buffer;
	}

	std::expected<void, util::Error> TransferBuffer::transfer(
		const std::function<void(void* mapped_ptr)>& callback,
		bool cycle
	) const noexcept
	{
		assert(callback != nullptr);
		assert(resource != nullptr);

		void* const mapped_ptr = SDL_MapGPUTransferBuffer(device, resource, cycle);
		if (mapped_ptr == nullptr) RETURN_SDL_ERROR;

		callback(mapped_ptr);
		SDL_UnmapGPUTransferBuffer(this->device, resource);

		return {};
	}

	std::expected<void, util::Error> TransferBuffer::upload_to_buffer(
		std::span<const std::byte> data,
		bool cycle
	)
	{
		assert(resource != nullptr);

		if (std::cmp_not_equal(data.size(), size))
			return util::Error(
				std::format("Can't upload {}B to a transfer buffer with size of {}B", size, data.size())
			);

		if (usage != Usage::Upload) return util::Error("Can't upload to a download-only transfer buffer");

		void* const mapped_ptr = SDL_MapGPUTransferBuffer(device, resource, cycle);
		if (mapped_ptr == nullptr) RETURN_SDL_ERROR;

		std::ranges::copy(data, reinterpret_cast<std::byte*>(mapped_ptr));
		SDL_UnmapGPUTransferBuffer(this->device, resource);

		return {};
	}

	std::expected<void, util::Error> TransferBuffer::download_from_buffer(std::span<std::byte> out_data) const
	{
		assert(resource != nullptr);

		if (std::cmp_not_equal(out_data.size(), size))
			return util::Error(
				std::format(
					"Can't download {}B from a transfer buffer with size of {}B",
					size,
					out_data.size()
				)
			);

		if (usage != Usage::Download)
			return util::Error("Can't download from an upload-only transfer buffer");

		const void* const mapped_ptr = SDL_MapGPUTransferBuffer(device, resource, false);
		if (mapped_ptr == nullptr) RETURN_SDL_ERROR;

		std::ranges::copy(std::span(reinterpret_cast<const std::byte*>(mapped_ptr), size), out_data.begin());
		SDL_UnmapGPUTransferBuffer(this->device, resource);

		return {};
	}

	std::expected<TransferBuffer, util::Error> TransferBuffer::create_from_data(
		SDL_GPUDevice* device,
		std::span<const std::byte> data
	) noexcept
	{
		auto transfer_buffer = create(device, Usage::Upload, static_cast<uint32_t>(data.size()));
		if (!transfer_buffer) return transfer_buffer.error().forward("Create transfer buffer failed");

		if (const auto upload_result = transfer_buffer->upload_to_buffer(data, false); !upload_result)
			return upload_result.error().forward("Upload data failed");

		return transfer_buffer;
	}

}