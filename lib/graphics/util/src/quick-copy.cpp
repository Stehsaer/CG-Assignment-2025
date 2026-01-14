#include "graphics/util/quick-copy.hpp"
#include "gpu/command-buffer.hpp"

namespace graphics
{
	std::expected<void, util::Error> execute_copy_task(
		SDL_GPUDevice* device,
		const std::function<void(const gpu::CopyPass&)>& task
	) noexcept
	{
		auto command_buffer = gpu::CommandBuffer::acquire_from(device);
		if (!command_buffer) return command_buffer.error().forward("Acquire command buffer failed");

		const auto copy_result = command_buffer->run_copy_pass(task);
		if (!copy_result) return copy_result.error().forward("Run copy pass failed");

		auto submit_result = command_buffer->submit_and_acquire_fence();
		if (!submit_result) return submit_result.error().forward("Submit command buffer failed");

		auto fence = std::move(*submit_result);
		auto wait_result = fence.wait();
		if (!wait_result) return wait_result.error().forward("Wait for fence failed");

		return {};
	}
}