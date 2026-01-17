#include "pipeline/surface.hpp"
#include "graphics/util/quick-create.hpp"
#include "shader/surface.hpp"
#include "target/msaa-draw.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/fwd.hpp>
#include <ranges>

namespace pipeline
{
	std::expected<Surface, util::Error> Surface::create(SDL_GPUDevice* device) noexcept
	{
		const auto shader_vertex = gpu::GraphicsShader::create(
			device,
			shader::surface,
			gpu::GraphicsShader::Stage::Vertex,
			0,
			0,
			0,
			1,
			"vs_main"
		);

		const auto shader_fragment = gpu::GraphicsShader::create(
			device,
			shader::surface,
			gpu::GraphicsShader::Stage::Fragment,
			0,
			0,
			0,
			0,
			"fs_main"
		);

		if (!shader_vertex) return shader_vertex.error().forward("Create surface vertex shader failed");
		if (!shader_fragment) return shader_fragment.error().forward("Create surface fragment shader failed");

		const auto swapchain_desc = SDL_GPUColorTargetDescription{
			.format = target::MSAADraw::COLOR_FORMAT.format,
			.blend_state = {
							.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
							.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
							.color_blend_op = SDL_GPU_BLENDOP_ADD,
							.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
							.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
							.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
							.color_write_mask = {},
							.enable_blend = false,
							.enable_color_write_mask = false,
							.padding1 = 0,
							.padding2 = 0
			},
		};
		const auto color_target_descs = std::to_array({swapchain_desc});

		const auto vertex_input_location0 = SDL_GPUVertexAttribute{
			.location = 0,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = 0
		};
		const auto vertex_inputs = std::to_array({vertex_input_location0});

		const auto vertex_buffer_slot0 = SDL_GPUVertexBufferDescription{
			.slot = 0,
			.pitch = sizeof(glm::vec2),
			.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
			.instance_step_rate = 0
		};
		const auto vertex_buffer_slots = std::to_array({vertex_buffer_slot0});

		const auto depth_stencil_state = gpu::GraphicsPipeline::DepthStencilState{
			.format = target::MSAADraw::DEPTH_FORMAT.format,
			.compare_op = SDL_GPU_COMPAREOP_LESS,
			.compare_mask = 0x00,
			.write_mask = 0x00,
			.enable_depth_test = true,
			.enable_depth_write = true,
			.enable_stencil_test = false,
		};

		auto wireframe_pipeline = gpu::GraphicsPipeline::create(
			device,
			*shader_vertex,
			*shader_fragment,
			SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
			target::MSAADraw::MSAA_SAMPLES,
			SDL_GPURasterizerState{
				.fill_mode = SDL_GPU_FILLMODE_LINE,
				.cull_mode = SDL_GPU_CULLMODE_NONE,
				.front_face = {},
				.depth_bias_constant_factor = 0,
				.depth_bias_clamp = 0,
				.depth_bias_slope_factor = 0,
				.enable_depth_bias = false,
				.enable_depth_clip = false,
				.padding1 = 0,
				.padding2 = 0
			},
			vertex_inputs,
			vertex_buffer_slots,
			color_target_descs,
			depth_stencil_state,
			"Surface Wireframe Pipeline"
		);
		auto solid_pipeline = gpu::GraphicsPipeline::create(
			device,
			*shader_vertex,
			*shader_fragment,
			SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
			target::MSAADraw::MSAA_SAMPLES,
			SDL_GPURasterizerState{
				.fill_mode = SDL_GPU_FILLMODE_FILL,
				.cull_mode = SDL_GPU_CULLMODE_NONE,
				.front_face = {},
				.depth_bias_constant_factor = 0,
				.depth_bias_clamp = 0,
				.depth_bias_slope_factor = 0,
				.enable_depth_bias = false,
				.enable_depth_clip = false,
				.padding1 = 0,
				.padding2 = 0
			},
			vertex_inputs,
			vertex_buffer_slots,
			color_target_descs,
			depth_stencil_state,
			"Surface Solid Pipeline"
		);

		if (!wireframe_pipeline)
			return wireframe_pipeline.error().forward("Create surface wireframe pipeline failed");
		if (!solid_pipeline) return solid_pipeline.error().forward("Create surface solid pipeline failed");

		const auto vertices =
			std::views::cartesian_product(
				std::views::iota(0u, SURFACE_RES),
				std::views::iota(0u, SURFACE_RES)
			)
			| std::views::transform([](const auto& pair) {
				  const auto [i, j] = pair;
				  const float u = i / float(SURFACE_RES - 1);
				  const float v = j / float(SURFACE_RES - 1);
				  return glm::vec2(u, v) * 2.0f - 1.0f;
			  })
			| std::ranges::to<std::vector>();

		const auto indices =
			std::views::cartesian_product(
				std::views::iota(0u, SURFACE_RES - 1),
				std::views::iota(0u, SURFACE_RES - 1)
			)
			| std::views::transform([](const auto& pair) {
				  const auto [i, j] = pair;
				  const uint32_t top_left = i * SURFACE_RES + j;
				  const uint32_t top_right = top_left + 1;
				  const uint32_t bottom_left = top_left + SURFACE_RES;
				  const uint32_t bottom_right = bottom_left + 1;

				  return std::to_array(
					  {top_left, bottom_left, top_right, top_right, bottom_left, bottom_right}
				  );
			  })
			| std::views::join
			| std::ranges::to<std::vector>();

		auto vertex_buffer = graphics::create_buffer_from_data(
			device,
			{.vertex = true},
			util::as_bytes(vertices),
			"Surface Vertex Buffer"
		);
		auto index_buffer = graphics::create_buffer_from_data(
			device,
			{.index = true},
			util::as_bytes(indices),
			"Surface Index Buffer"
		);

		if (!vertex_buffer) return vertex_buffer.error().forward("Create surface vertex buffer failed");
		if (!index_buffer) return index_buffer.error().forward("Create surface index buffer failed");

		return Surface(
			std::move(*solid_pipeline),
			std::move(*wireframe_pipeline),
			std::move(*vertex_buffer),
			std::move(*index_buffer)
		);
	}

	void Surface::draw(
		const gpu::CommandBuffer& command_buffer,
		const gpu::RenderPass& render_pass,
		const Surface::Params& params,
		bool wireframe
	) const noexcept
	{
		command_buffer.push_uniform_to_vertex(0, util::as_bytes(params));
		if (wireframe)
			render_pass.bind_pipeline(wireframe_pipeline);
		else
			render_pass.bind_pipeline(solid_pipeline);

		render_pass.bind_vertex_buffers(0, SDL_GPUBufferBinding{.buffer = vertex_buffer, .offset = 0});
		render_pass.bind_index_buffer(
			SDL_GPUBufferBinding{
				.buffer = index_buffer,
				.offset = 0,
			},
			SDL_GPU_INDEXELEMENTSIZE_32BIT
		);

		render_pass.draw_indexed((SURFACE_RES - 1) * (SURFACE_RES - 1) * 6, 0, 1, 0, 0);
	}
}