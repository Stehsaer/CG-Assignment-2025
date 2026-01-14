///
/// @file imgui.hpp
/// @brief Provides ImGui initialization and rendering functions
/// @details
/// #### Initialization
/// Call `initialize_imgui` after creating the SDL context to set up ImGui.
///
/// #### Usage
/// 1. At event handling loop, call `imgui_handle_event` for each SDL event.
/// 2. Before acquiring the swapchain, call `imgui_new_frame` to start a new ImGui frame.
/// 3. Create UI elements...
/// 4. Create a command buffer
/// 5. Call `imgui_upload_data` on the command buffer to upload ImGui data to the GPU.
/// 6. During the final render pass, call `imgui_draw_to_renderpass` to render ImGui elements into the
/// swapchain.
///

#pragma once

#include <SDL3/SDL_events.h>
#include <imgui.h>

#include "gpu/command-buffer.hpp"
#include "gpu/render-pass.hpp"
#include "sdl.hpp"

namespace backend
{
	///
	/// @brief Initialize ImGui context
	///
	/// @param sdl_context SDL Context
	///
	std::expected<void, util::Error> initialize_imgui(const SDLcontext& sdl_context) noexcept;

	///
	/// @brief Destroy ImGui context
	///
	///
	void destroy_imgui() noexcept;

	///
	/// @brief Pass an SDL event to ImGui for processing
	/// @note Call this function for each SDL event in the event loop.
	/// @param event SDL event
	///
	void imgui_handle_event(const SDL_Event* event) noexcept;

	///
	/// @brief Begin a new ImGui frame
	/// @note Call this function before acquiring the swapchain to get better performance.
	///
	void imgui_new_frame() noexcept;

	///
	/// @brief Upload ImGui data to the GPU
	/// @note Call this function on a command buffer before rendering ImGui elements.
	///
	void imgui_upload_data(const gpu::CommandBuffer& command_buffer) noexcept;

	///
	/// @brief Render ImGui draw data into a render pass
	/// @note Generally called during the final render pass to render ImGui elements into the swapchain.
	///
	void imgui_draw_to_renderpass(
		const gpu::CommandBuffer& command_buffer,
		const gpu::RenderPass& render_pass
	) noexcept;
}