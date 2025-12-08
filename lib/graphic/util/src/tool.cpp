#include "graphic/util/tool.hpp"

namespace graphic
{
	std::expected<void, util::Error> execute_copy_task(
		SDL_GPUDevice* device,
		const std::function<void(const gpu::Copy_pass&)>& task
	) noexcept
	{
		auto command_buffer = gpu::Command_buffer::acquire_from(device);
		if (!command_buffer) return command_buffer.error().propagate("Acquire command buffer failed");

		const auto copy_result = command_buffer->run_copy_pass(task);
		if (!copy_result) return copy_result.error().propagate("Run copy pass failed");

		const auto submit_result = command_buffer->submit();
		if (!submit_result) return submit_result.error().propagate("Submit command buffer failed");

		return {};
	}

	std::expected<gpu::Buffer, util::Error> create_buffer_with_data(
		SDL_GPUDevice* device,
		gpu::Buffer::Usage usage,
		std::span<const std::byte> data
	) noexcept
	{
		auto buffer = gpu::Buffer::create(device, usage, data.size());
		if (!buffer) return buffer.error().propagate("Create buffer failed");

		auto transfer_buffer = gpu::Transfer_buffer::create_from_data(device, data);
		if (!transfer_buffer) return transfer_buffer.error().propagate("Create transfer buffer failed");

		const auto copy_result = execute_copy_task(device, [&](const gpu::Copy_pass& copy_pass) {
			copy_pass.upload_to_buffer(*transfer_buffer, 0, *buffer, 0, data.size(), false);
		});

		if (!copy_result) return copy_result.error().propagate("Execute copy task failed");

		return buffer;
	}
}