#include "backend/imgui.hpp"
#include "backend/sdl.hpp"
#include "capsule-ui.hpp"
#include "geometry/primitive.hpp"
#include "gpu/command-buffer.hpp"
#include "gpu/render-pass.hpp"
#include "graphics/util/quick-create.hpp"
#include "pipeline/line.hpp"
#include "util/as-byte.hpp"
#include "util/error.hpp"
#include "util/unwrap.hpp"

#include <SDL3/SDL_gpu.h>
#include <print>

int main()
{
	const primitive::BezierCurve test_curve{
		.color = {255, 255, 0, 255},
		.control_points = {{-0.5, -0.5}, {-0.5, 0.5}, {0.5, 0.5}, {0.5, -0.5}, {1, 1}}
	};
	const auto curve_vertices = test_curve.gen_vertices();

	try
	{
		const auto sdl_context = backend::SDLcontext::create(1280, 720, "Homework 3") | util::unwrap();
		backend::initialize_imgui(*sdl_context) | util::unwrap();
		const auto pipeline_format = sdl_context->get_swapchain_texture_format();
		const auto line_pipeline =
			pipeline::Line::create(sdl_context->device, pipeline_format) | util::unwrap();
		const auto test_buffer =
			graphics::create_buffer_from_data(
				sdl_context->device,
				{.vertex = true},
				util::as_bytes(curve_vertices),
				""
			)
			| util::unwrap();

		while (true)
		{
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				backend::imgui_handle_event(&event);
				if (event.type == SDL_EVENT_QUIT) return EXIT_SUCCESS;
			}

			backend::imgui_new_frame();

			capsule::window("Test", capsule::Position::BottomCenter, [] { capsule::button("w"); });

			auto command_buffer = gpu::CommandBuffer::acquire_from(sdl_context->device) | util::unwrap();

			auto swapchain = command_buffer.acquire_swapchain_texture(sdl_context->window) | util::unwrap();
			if (!swapchain.has_value()) continue;

			backend::imgui_upload_data(command_buffer);

			const auto swapchain_color_target_info = SDL_GPUColorTargetInfo{
				.texture = swapchain->swapchain_texture,
				.mip_level = 0,
				.layer_or_depth_plane = 0,
				.clear_color = {.r = 0, .g = 0, .b = 0, .a = 255},
				.load_op = SDL_GPU_LOADOP_CLEAR,
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

					const auto vp_matrix = glm::mat4(1.0f);
					command_buffer.push_uniform_to_vertex(0, util::as_bytes(vp_matrix));

					render_pass.bind_pipeline(line_pipeline.pipeline);
					render_pass
						.bind_vertex_buffers(0, SDL_GPUBufferBinding{.buffer = test_buffer, .offset = 0});

					render_pass.draw(curve_vertices.size(), 0, 1, 0);
				}
			) | util::unwrap();

			command_buffer.submit() | util::unwrap();
		}

		return EXIT_SUCCESS;
	}
	catch (const util::Error& err)
	{
		err.dump_trace();
		return EXIT_FAILURE;
	}
	catch (const std::exception& e)
	{
		std::println("Error: {}", e.what());
		return EXIT_FAILURE;
	}
}