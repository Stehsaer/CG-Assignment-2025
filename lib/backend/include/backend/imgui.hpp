#pragma once

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <gpu/command-buffer.hpp>
#include <gpu/render-pass.hpp>
#include <imgui.h>

#include "sdl.hpp"

namespace backend
{
	///
	/// @brief Initialize the ImGui library
	///
	/// @param sdl_context SDL Context
	///
	std::expected<void, util::Error> initialize_imgui(const SDL_context& sdl_context) noexcept;

	///
	/// @brief Pass an SDL event to ImGui for processing
	///
	/// @param event SDL event
	///
	void imgui_handle_event(const SDL_Event* event) noexcept;

	///
	/// @brief Begin a new ImGui frame
	/// @note Call this function before acquiring the swapchain to get better perf.
	///
	void imgui_new_frame() noexcept;

	///
	/// @brief Upload ImGui data to the GPU
	///
	///
	void imgui_upload_data(const gpu::Command_buffer& command_buffer) noexcept;

	///
	/// @brief Render ImGui draw data into a render pass
	///
	void imgui_draw_to_renderpass(
		const gpu::Command_buffer& command_buffer,
		const gpu::Render_pass& render_pass
	) noexcept;
}