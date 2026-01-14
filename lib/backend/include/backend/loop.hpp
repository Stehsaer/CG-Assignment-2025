///
/// @file loop.hpp
/// @brief Provides helper functions for running the main loop
///

#pragma once

#include <functional>
#include <future>
#include <glm/glm.hpp>
#include <imgui.h>
#include <thread>

#include "gpu/command-buffer.hpp"
#include "sdl.hpp"

namespace backend
{
	///
	/// @brief Run one frame of the main loop
	///
	/// @param loop_fn Loop function, returns `true` to continue, `false` to exit
	/// @param render_fn Render function
	/// @return `true` if the loop should continue, `false` to exit, or an error if something went wrong
	///
	std::expected<bool, util::Error> run_one_frame(
		const SDLcontext& context,
		bool clear,
		const std::function<bool()>& loop_fn,
		const std::function<std::expected<void, util::Error>(
			const gpu::CommandBuffer& command_buffer,
			SDL_GPUTexture* swapchain,
			glm::u32vec2 size
		)>& render_fn
	);

	///
	/// @brief Helper function that displays a progress window until the given future is done
	/// @details This function takes the ownership of the future, displays the UI while waiting for it to
	/// complete, and returns the result
	///
	/// @tparam T The type of the future's result
	/// @param context SDL context
	/// @param future The future to wait for.
	/// @param progress_display_fn Function to display progress. Called every frame. The ImGui window is
	/// already created before the call.
	/// @return The result of the future
	///
	template <typename T>
	T display_until_task_done(
		const SDLcontext& context,
		std::future<T> future,
		const std::function<void()>& progress_display_fn
	)
	{
		const auto frame_fn = [&progress_display_fn] -> bool {
			const auto size = ImGui::GetIO().DisplaySize;
			ImGui::SetNextWindowPos(
				ImVec2(size.x * 0.5f, size.y * 0.5f),
				ImGuiCond_Always,
				ImVec2(0.5f, 0.5f)
			);

			if (ImGui::Begin(
					"##Loading",
					nullptr,
					ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
				))
				progress_display_fn();

			ImGui::End();
			return true;
		};

		while (!future.valid()) std::this_thread::yield();

		while (future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
			if (!run_one_frame(context, true, frame_fn, nullptr).has_value()) std::terminate();

		return future.get();
	}
}