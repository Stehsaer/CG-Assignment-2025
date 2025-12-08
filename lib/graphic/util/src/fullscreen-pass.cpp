#include "graphic/util/fullscreen-pass.hpp"
#include "asset/shader/fullscreen.vert.hpp"
#include "graphic/util/tool.hpp"

#include <glm/glm.hpp>
#include <util/as-byte.hpp>

namespace graphic
{
	namespace
	{
		constexpr SDL_GPURasterizerState rasterizer_state = {
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.cull_mode = SDL_GPU_CULLMODE_NONE,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
			.depth_bias_constant_factor = 0,
			.depth_bias_clamp = 0,
			.depth_bias_slope_factor = 0,
			.enable_depth_bias = false,
			.enable_depth_clip = false,
			.padding1 = 0,
			.padding2 = 0
		};

		constexpr std::array vertex_attributes = std::to_array<SDL_GPUVertexAttribute>({
			{.location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = 0}
		});

		constexpr std::array vertex_buffer_descs = std::to_array<SDL_GPUVertexBufferDescription>({
			{.slot = 0,
			 .pitch = sizeof(glm::vec2),
			 .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
			 .instance_step_rate = 0}
		});

		constexpr auto fullscreen_triangle_vertices = std::to_array<glm::vec2>({
			{-1.0f, -1.0f},
			{-1.0f, 1.0f },
			{1.0f,  -1.0f},
			{1.0f,  1.0f },
		});

		std::expected<gpu::Buffer, util::Error> create_fullscreen_vertex_buffer(
			SDL_GPUDevice* device
		) noexcept
		{
			auto vertex_buffer = gpu::Buffer::create(
				device,
				{.vertex = true},
				sizeof(glm::vec2) * fullscreen_triangle_vertices.size()
			);
			if (!vertex_buffer) return vertex_buffer.error().propagate("Create vertex buffer failed");

			auto transfer_buffer =
				gpu::Transfer_buffer::create_from_data(device, util::as_bytes(fullscreen_triangle_vertices));
			if (!transfer_buffer) return transfer_buffer.error().propagate("Create transfer buffer failed");

			const auto copy_result = execute_copy_task(device, [&](const gpu::Copy_pass& copy_pass) {
				copy_pass.upload_to_buffer(
					*transfer_buffer,
					0,
					*vertex_buffer,
					0,
					sizeof(glm::vec2) * fullscreen_triangle_vertices.size(),
					false
				);
			});
			if (!copy_result) return copy_result.error().propagate("Upload vertex data failed");

			return vertex_buffer;
		}

		std::expected<gpu::Graphics_pipeline, util::Error> create_fullscreen_pass(
			SDL_GPUDevice* device,
			const gpu::Graphic_shader& fragment,
			gpu::Texture::Format target_format
		) noexcept
		{
			if (target_format.type != SDL_GPU_TEXTURETYPE_2D
				&& target_format.type != SDL_GPU_TEXTURETYPE_CUBE)
				return util::Error("2D or Cube texture format required for fullscreen pass");

			if (!target_format.usage.color_target)
				return util::Error("Target format should support color target usage");

			const std::array color_target_descs = std::to_array<SDL_GPUColorTargetDescription>({
				{.format = target_format.format,
				 .blend_state = {
					 .src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
					 .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
					 .color_blend_op = SDL_GPU_BLENDOP_ADD,
					 .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
					 .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
					 .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
					 .color_write_mask = SDL_GPU_COLORCOMPONENT_R
						 | SDL_GPU_COLORCOMPONENT_G
						 | SDL_GPU_COLORCOMPONENT_B
						 | SDL_GPU_COLORCOMPONENT_A,
					 .enable_blend = false,
					 .enable_color_write_mask = false,
					 .padding1 = 0,
					 .padding2 = 0
				 }}
			});

			auto vertex_shader = gpu::Graphic_shader::
				create(device, shader_asset::fullscreen_vert, gpu::Graphic_shader::Stage::Vertex, 0, 0, 0, 0);
			if (!vertex_shader) return vertex_shader.error().propagate("Create vertex shader failed");

			auto create_pipeline_result = gpu::Graphics_pipeline::create(
				device,
				*vertex_shader,
				fragment,
				SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
				SDL_GPU_SAMPLECOUNT_1,
				rasterizer_state,
				vertex_attributes,
				vertex_buffer_descs,
				color_target_descs,
				std::nullopt
			);
			if (!create_pipeline_result)
				return create_pipeline_result.error().propagate("Create pipeline failed");

			return std::move(create_pipeline_result.value());
		}

	}  // namespace

	Fullscreen_pass::Fullscreen_pass(
		gpu::Graphics_pipeline pipeline,
		gpu::Buffer vertex_buffer,
		Config config
	) noexcept :
		fullscreen_vertex_buffer(std::move(vertex_buffer)),
		pipeline(std::move(pipeline)),
		config(config)
	{}

	std::expected<Fullscreen_pass, util::Error> Fullscreen_pass::create(
		SDL_GPUDevice* device,
		const gpu::Graphic_shader& fragment,
		gpu::Texture::Format target_format,
		Config config
	) noexcept
	{
		auto create_vertex_buffer_result = create_fullscreen_vertex_buffer(device);
		if (!create_vertex_buffer_result)
			return create_vertex_buffer_result.error().propagate("创建顶点缓冲区失败");
		auto vertex_buffer = std::move(create_vertex_buffer_result.value());

		auto create_pipeline_result = create_fullscreen_pass(device, fragment, target_format);
		if (!create_pipeline_result) return create_pipeline_result.error().propagate("创建管线失败");
		auto pipeline = std::move(create_pipeline_result.value());

		return Fullscreen_pass(std::move(pipeline), std::move(vertex_buffer), config);
	}

	std::expected<void, util::Error> Fullscreen_pass::render(
		const gpu::Command_buffer& command_buffer,
		SDL_GPUTexture* target_texture,
		std::optional<std::span<const SDL_GPUTextureSamplerBinding>> samplers,
		std::optional<std::span<SDL_GPUTexture* const>> storage_textures,
		std::optional<std::span<SDL_GPUBuffer* const>> storage_buffers
	) noexcept
	{
		const auto vertex_buffer_bindings = std::to_array<SDL_GPUBufferBinding>({
			{.buffer = fullscreen_vertex_buffer, .offset = 0}
		});

		const auto color_target_info = std::to_array<SDL_GPUColorTargetInfo>({
			{.texture = target_texture,
			 .mip_level = 0,
			 .layer_or_depth_plane = 0,
			 .clear_color =
				 {.r = config.clear_color.r,
				  .g = config.clear_color.g,
				  .b = config.clear_color.b,
				  .a = config.clear_color.a},
			 .load_op = config.clear_before_render ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_DONT_CARE,
			 .store_op = SDL_GPU_STOREOP_STORE,
			 .resolve_texture = nullptr,
			 .resolve_mip_level = 0,
			 .resolve_layer = 0,
			 .cycle = config.do_cycle,
			 .cycle_resolve_texture = false,
			 .padding1 = 0,
			 .padding2 = 0}
		});

		const auto render_result = command_buffer.run_render_pass(
			color_target_info,
			std::nullopt,
			[&](const gpu::Render_pass& render_pass) {
				render_pass.bind_pipeline(pipeline);

				render_pass.bind_vertex_buffers(0, vertex_buffer_bindings);

				if (samplers.has_value()) render_pass.bind_fragment_samplers(0, samplers.value());
				if (storage_textures.has_value())
					render_pass.bind_fragment_storage_textures(0, storage_textures.value());
				if (storage_buffers.has_value())
					render_pass.bind_fragment_storage_buffers(0, storage_buffers.value());

				render_pass.draw(4, 0, 1, 0);
			}
		);

		return render_result;
	}

}