#include "backend/loop.hpp"
#include "backend/imgui.hpp"
#include "backend/sdl.hpp"

namespace backend
{
	static SDL_GPUColorTargetInfo gen_swapchain_target_info(SDL_GPUTexture* swapchain, bool clear) noexcept
	{
		SDL_GPUColorTargetInfo swapchain_target;
		swapchain_target.texture = swapchain;
		swapchain_target.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
		swapchain_target.store_op = SDL_GPU_STOREOP_STORE;
		swapchain_target.clear_color = SDL_FColor{.r = 0, .g = 0, .b = 0, .a = 1};
		swapchain_target.resolve_texture = nullptr;
		swapchain_target.cycle = false;
		swapchain_target.mip_level = 0;
		swapchain_target.layer_or_depth_plane = 0;

		return swapchain_target;
	}

	std::expected<bool, util::Error> run_one_frame(
		const SDLcontext& context,
		bool clear,
		const std::function<bool()>& loop_fn,
		const std::function<std::expected<void, util::Error>(
			const gpu::CommandBuffer& command_buffer,
			SDL_GPUTexture* swapchain,
			glm::u32vec2 size
		)>& render_fn
	)
	{
		bool should_continue = true;

		/* Process SDL Events */

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			backend::imgui_handle_event(&event);
			if (event.type == SDL_EVENT_QUIT) should_continue = false;
		}

		/* ImGui Frame Logic */

		backend::imgui_new_frame();
		should_continue &= (loop_fn == nullptr ? true : loop_fn());

		/* Acquire Swapchain */

		auto command_buffer = gpu::CommandBuffer::acquire_from(context.device);
		if (!command_buffer) return command_buffer.error().forward("Acquire command buffer failed");

		const auto swapchain_result = command_buffer->wait_and_acquire_swapchain_texture(context.window);
		if (!swapchain_result) return swapchain_result.error().forward("Acquire swapchain texture failed");
		const auto [swapchain_texture, width, height] = *swapchain_result;

		/* Upload ImGui Data */

		backend::imgui_upload_data(*command_buffer);

		/* Render Custom Logic */

		if (render_fn != nullptr)
		{
			const auto render_result = render_fn(*command_buffer, swapchain_texture, {width, height});
			if (!render_result) return render_result.error().forward("Render function failed");
		}

		/* Render ImGui Content */

		const auto swapchain_info = gen_swapchain_target_info(swapchain_texture, clear);
		const auto render_imgui_result = command_buffer->run_render_pass(
			{&swapchain_info, 1},
			{},
			[&command_buffer](const gpu::RenderPass& render_pass) {
				backend::imgui_draw_to_renderpass(*command_buffer, render_pass);
			}
		);
		if (!render_imgui_result) return render_imgui_result.error().forward("Render ImGui failed");

		/* Submit Command Buffer */

		const auto submit_result = command_buffer->submit();
		if (!submit_result) return submit_result.error().forward("Submit command buffer failed");

		return should_continue;
	}
}