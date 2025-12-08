#pragma once

#include <functional>
#include <glm/glm.hpp>
#include <gpu/command-buffer.hpp>

#include "sdl.hpp"

namespace backend
{
	using UI_func = std::function<bool()>;

	using Render_func = std::function<std::expected<void, util::Error>(
		const gpu::Command_buffer& command_buffer,
		SDL_GPUTexture* swapchain,
		glm::u32vec2 size
	)>;

	///
	/// @brief Run one frame of the main loop
	///
	/// @param loop_fn Loop function
	/// @param render_fn Render function
	/// @return `true` if the loop should continue, `false` to exit, or an error if something went wrong
	///
	std::expected<bool, util::Error> run_one_frame(
		const SDL_context& context,
		const UI_func& loop_fn,
		const Render_func& render_fn
	);
}