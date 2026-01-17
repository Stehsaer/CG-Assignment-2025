#include "logic/dynamic-gpu-buf.hpp"

#include <glm/glm.hpp>

namespace logic
{
	void DynamicGPUBuffer::copy_to_gpu(const gpu::CopyPass& copy_pass) noexcept
	{
		if (size == 0) return;
		if (!transfer_buffer.has_value() || !buffer.has_value()) return;

		if (!buffers_synchronized)
		{
			copy_pass.upload_to_buffer(*transfer_buffer, 0, *buffer, 0, size, true);
			buffers_synchronized = true;
		}
	}

	std::expected<void, util::Error> DynamicGPUBuffer::write_transfer(
		SDL_GPUDevice* device,
		std::span<const std::byte> data
	) noexcept
	{
		if (data.empty()) return {};

		buffers_synchronized = false;

		if (data.size() > capacity || !transfer_buffer.has_value() || !buffer.has_value())
		{
			capacity = 1 << int(glm::ceil(glm::log2(float(data.size()))));

			transfer_buffer.reset();
			buffer.reset();

			auto new_transfer_buffer =
				gpu::TransferBuffer::create(device, gpu::TransferBuffer::Usage::Upload, capacity);
			if (!new_transfer_buffer.has_value())
				return new_transfer_buffer.error().forward("Create transfer buffer failed");
			transfer_buffer = std::move(new_transfer_buffer.value());

			auto new_buffer = gpu::Buffer::create(
				device,
				{.vertex = usage_vertex, .indirect = usage_indirect},
				capacity,
				"Dynamic GPU Buffer"
			);
			if (!new_buffer.has_value()) return new_buffer.error().forward("Create GPU buffer failed");
			buffer = std::move(new_buffer.value());
		}

		const auto upload_result = transfer_buffer->upload_to_buffer(data, true);
		if (!upload_result.has_value())
			return upload_result.error().forward("Upload to dynamic GPU buffer failed");

		size = data.size();

		return {};
	}
}