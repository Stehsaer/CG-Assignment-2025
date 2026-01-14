#pragma once

#include "gpu/copy-pass.hpp"

#include <expected>
#include <functional>

namespace graphics
{
	///
	/// @brief Create a command buffer, perform a copy task, submit and wait for completion.
	///
	/// @param task Copy task
	///
	std::expected<void, util::Error> execute_copy_task(
		SDL_GPUDevice* device,
		const std::function<void(const gpu::CopyPass&)>& task
	) noexcept;
}