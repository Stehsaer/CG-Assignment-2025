#include "render.hpp"
#include "backend/imgui.hpp"
#include "util/unwrap.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>

RenderManager RenderManager::create()
{
	auto sdl_context =
		backend::SDLcontext::create(1280, 720, "Homework 3", SDL_WINDOW_RESIZABLE) | util::unwrap();
	backend::initialize_imgui(*sdl_context) | util::unwrap();

	auto line_pipeline = pipeline::Line::create(sdl_context->device) | util::unwrap();
	auto surface_pipeline = pipeline::Surface::create(sdl_context->device) | util::unwrap();
	auto msaa_buffer = target::MSAADraw();

	return {
		std::move(sdl_context),
		std::move(line_pipeline),
		std::move(surface_pipeline),
		std::move(msaa_buffer)
	};
}

bool RenderManager::run_frame()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		backend::imgui_handle_event(&event);
		if (event.type == SDL_EVENT_QUIT) return false;
	}

	backend::imgui_new_frame();

	app.imgui_frame(sdl_context->device) | util::unwrap();

	auto command_buffer = gpu::CommandBuffer::acquire_from(sdl_context->device) | util::unwrap();

	auto swapchain = command_buffer.acquire_swapchain_texture(sdl_context->window) | util::unwrap();
	if (!swapchain.has_value() || swapchain->width < 100 || swapchain->height < 100) return true;

	msaa_buffer.resize(sdl_context->device, {swapchain->width, swapchain->height}) | util::unwrap();
	backend::imgui_upload_data(command_buffer);

	command_buffer.run_copy_pass([&](const gpu::CopyPass& copy_pass) {
		app.upload_frame(copy_pass);
	}) | util::unwrap();

	const auto msaa_color_target_info = SDL_GPUColorTargetInfo{
		.texture = *msaa_buffer.texture,
		.mip_level = 0,
		.layer_or_depth_plane = 0,
		.clear_color = {.r = 0, .g = 0, .b = 0, .a = 255},
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_RESOLVE,
		.resolve_texture = swapchain->swapchain_texture,
		.resolve_mip_level = 0,
		.resolve_layer = 0,
		.cycle = true,
		.cycle_resolve_texture = false,
		.padding1 = 0,
		.padding2 = 0
	};
	const auto msaa_color_target_info_list = std::to_array({msaa_color_target_info});

	const auto msaa_depth_stencil_target_info = SDL_GPUDepthStencilTargetInfo{
		.texture = *msaa_buffer.depth_texture,
		.clear_depth = 1.0f,
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE,
		.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
		.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
		.cycle = true,
		.clear_stencil = 0,
		.mip_level = 0,
		.layer = 0
	};

	command_buffer.run_render_pass(
		msaa_color_target_info_list,
		msaa_depth_stencil_target_info,
		[&, this](const gpu::RenderPass& render_pass) {
			app.draw_frame(line_pipeline.pipeline, surface_pipeline, command_buffer, render_pass);
		}
	) | util::unwrap();

	const auto swapchain_color_target_info = SDL_GPUColorTargetInfo{
		.texture = swapchain->swapchain_texture,
		.mip_level = 0,
		.layer_or_depth_plane = 0,
		.clear_color = {.r = 0, .g = 0, .b = 0, .a = 255},
		.load_op = SDL_GPU_LOADOP_LOAD,
		.store_op = SDL_GPU_STOREOP_STORE,
		.resolve_texture = nullptr,
		.resolve_mip_level = 0,
		.resolve_layer = 0,
		.cycle = false,
		.cycle_resolve_texture = false,
		.padding1 = 0,
		.padding2 = 0
	};
	const auto color_target_info_list = std::to_array({swapchain_color_target_info});

	command_buffer.run_render_pass(
		color_target_info_list,
		std::nullopt,
		[&](const gpu::RenderPass& render_pass) {
			backend::imgui_draw_to_renderpass(command_buffer, render_pass);
		}
	) | util::unwrap();

	command_buffer.submit() | util::unwrap();

	return true;
}