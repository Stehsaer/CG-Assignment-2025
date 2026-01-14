#include "graphics/util/fullscreen-pass.hpp"
#include "asset/shader/fullscreen.vert.hpp"
#include "graphics/util/quick-copy.hpp"

#include "util/as-byte.hpp"
#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

namespace graphics
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

		const std::array vertex_attributes = std::to_array<SDL_GPUVertexAttribute>({
			{.location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = 0}
		});

		const std::array vertex_buffer_descs = std::to_array<SDL_GPUVertexBufferDescription>({
			{.slot = 0,
			 .pitch = sizeof(glm::vec2),
			 .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
			 .instance_step_rate = 0}
		});

		const auto fullscreen_triangle_vertices = std::to_array<glm::vec2>({
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
				sizeof(glm::vec2) * fullscreen_triangle_vertices.size(),
				"Fullscreen Pass Vertex Buffer"
			);
			if (!vertex_buffer) return vertex_buffer.error().forward("Create vertex buffer failed");

			auto transfer_buffer =
				gpu::TransferBuffer::create_from_data(device, util::as_bytes(fullscreen_triangle_vertices));
			if (!transfer_buffer) return transfer_buffer.error().forward("Create transfer buffer failed");

			const auto copy_result = execute_copy_task(device, [&](const gpu::CopyPass& copy_pass) {
				copy_pass.upload_to_buffer(
					*transfer_buffer,
					0,
					*vertex_buffer,
					0,
					sizeof(glm::vec2) * fullscreen_triangle_vertices.size(),
					false
				);
			});
			if (!copy_result) return copy_result.error().forward("Upload vertex data failed");

			return vertex_buffer;
		}

		SDL_GPUColorTargetBlendState color_target_blend_state_by_mode(FullscreenBlendMode mode) noexcept
		{
			switch (mode)
			{
			case FullscreenBlendMode::Overwrite:
				return {
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
					.enable_color_write_mask = true,
					.padding1 = 0,
					.padding2 = 0
				};
			case FullscreenBlendMode::Add:
				return {
					.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
					.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
					.color_blend_op = SDL_GPU_BLENDOP_ADD,
					.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
					.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
					.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
					.color_write_mask = SDL_GPU_COLORCOMPONENT_R
						| SDL_GPU_COLORCOMPONENT_G
						| SDL_GPU_COLORCOMPONENT_B
						| SDL_GPU_COLORCOMPONENT_A,
					.enable_blend = true,
					.enable_color_write_mask = true,
					.padding1 = 0,
					.padding2 = 0
				};
			default:
				std::unreachable();
			}
		}

		std::expected<gpu::GraphicsPipeline, util::Error> create_fullscreen_pass(
			SDL_GPUDevice* device,
			const gpu::GraphicsShader& fragment,
			gpu::Texture::Format target_format,
			FullscreenBlendMode blend_mode,
			std::optional<FullscreenStencilState> stencil_state,
			const std::string& name
		) noexcept
		{
			if (target_format.type != SDL_GPU_TEXTURETYPE_2D
				&& target_format.type != SDL_GPU_TEXTURETYPE_CUBE)
				return util::Error("2D or Cube texture format required for fullscreen pass");

			if (!target_format.usage.color_target)
				return util::Error("Target format should support color target usage");

			const std::array color_target_descs = std::to_array<SDL_GPUColorTargetDescription>({
				{.format = target_format.format, .blend_state = color_target_blend_state_by_mode(blend_mode)}
			});

			auto vertex_shader = gpu::GraphicsShader::create(
				device,
				shader_asset::fullscreen_vert,
				gpu::GraphicsShader::Stage::Vertex,
				0,
				0,
				0,
				0
			);
			if (!vertex_shader) return vertex_shader.error().forward("Create vertex shader failed");

			auto create_pipeline_result = gpu::GraphicsPipeline::create(
				device,
				*vertex_shader,
				fragment,
				SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
				SDL_GPU_SAMPLECOUNT_1,
				rasterizer_state,
				vertex_attributes,
				vertex_buffer_descs,
				color_target_descs,
				stencil_state.transform([](const auto& state) { return state.to_depth_stencil_state(); }),
				name
			);
			if (!create_pipeline_result)
				return create_pipeline_result.error().forward("Create pipeline failed");

			return std::move(create_pipeline_result.value());
		}

	}  // namespace

	gpu::GraphicsPipeline::DepthStencilState FullscreenStencilState::to_depth_stencil_state() const noexcept
	{
		const auto stencil_op_state = SDL_GPUStencilOpState{
			.fail_op = SDL_GPU_STENCILOP_KEEP,
			.pass_op = SDL_GPU_STENCILOP_KEEP,
			.depth_fail_op = SDL_GPU_STENCILOP_KEEP,
			.compare_op = compare_op
		};

		return gpu::GraphicsPipeline::DepthStencilState{
			.format = depth_format,
			.compare_op = SDL_GPU_COMPAREOP_ALWAYS,
			.back_stencil_state = stencil_op_state,
			.front_stencil_state = stencil_op_state,
			.compare_mask = compare_mask,
			.write_mask = write_mask,
			.enable_depth_test = false,
			.enable_depth_write = false,
			.enable_stencil_test = true
		};
	}

	void FullscreenPass<false>::render_to_renderpass(
		const gpu::RenderPass& render_pass,
		std::optional<std::span<const SDL_GPUTextureSamplerBinding>> samplers,
		std::optional<std::span<SDL_GPUTexture* const>> storage_textures,
		std::optional<std::span<SDL_GPUBuffer* const>> storage_buffers
	) const noexcept
	{
		render_pass.bind_pipeline(pipeline);

		render_pass
			.bind_vertex_buffers(0, SDL_GPUBufferBinding{.buffer = fullscreen_vertex_buffer, .offset = 0});

		if (samplers.has_value()) render_pass.bind_fragment_samplers(0, samplers.value());
		if (storage_textures.has_value())
			render_pass.bind_fragment_storage_textures(0, storage_textures.value());
		if (storage_buffers.has_value())
			render_pass.bind_fragment_storage_buffers(0, storage_buffers.value());
		if (stencil_ref.has_value()) render_pass.set_stencil_reference(*stencil_ref);

		render_pass.draw(4, 0, 1, 0);
	}

	std::expected<FullscreenPass<false>, util::Error> FullscreenPass<false>::create(
		SDL_GPUDevice* device,
		const gpu::GraphicsShader& fragment,
		gpu::Texture::Format target_format,
		const std::string& name,
		FullscreenBlendMode mode,
		std::optional<FullscreenStencilState> stencil_state
	) noexcept
	{
		auto create_vertex_buffer_result = create_fullscreen_vertex_buffer(device);
		if (!create_vertex_buffer_result)
			return create_vertex_buffer_result.error().forward("Create vertex buffer failed");
		auto vertex_buffer = std::move(create_vertex_buffer_result.value());

		auto create_pipeline_result =
			create_fullscreen_pass(device, fragment, target_format, mode, stencil_state, name);
		if (!create_pipeline_result) return create_pipeline_result.error().forward("Create pipeline failed");
		auto pipeline = std::move(create_pipeline_result.value());

		return FullscreenPass<false>(
			std::move(pipeline),
			std::move(vertex_buffer),
			stencil_state.transform([](const auto& state) { return state.reference; })
		);
	}

	std::expected<FullscreenPass<true>, util::Error> FullscreenPass<true>::create(
		SDL_GPUDevice* device,
		const gpu::GraphicsShader& fragment,
		gpu::Texture::Format target_format,
		Config config,
		const std::string& name
	) noexcept
	{
		auto create_base_pass = FullscreenPass<false>::create(
			device,
			fragment,
			target_format,
			name,
			config.blend_mode,
			config.stencil_state
		);
		if (!create_base_pass) return create_base_pass.error().forward("Create base fullscreen pass failed");

		return FullscreenPass<true>(std::move(*create_base_pass), config);
	}

	SDL_GPULoadOp FullscreenPass<true>::select_loadop() const noexcept
	{
		if (config.clear_before_render)
			return SDL_GPU_LOADOP_CLEAR;
		else
			return config.do_cycle ? SDL_GPU_LOADOP_DONT_CARE : SDL_GPU_LOADOP_LOAD;
	}

	std::expected<void, util::Error> FullscreenPass<true>::render(
		const gpu::CommandBuffer& command_buffer,
		SDL_GPUTexture* target_texture,
		std::optional<std::span<const SDL_GPUTextureSamplerBinding>> samplers,
		std::optional<std::span<SDL_GPUTexture* const>> storage_textures,
		std::optional<std::span<SDL_GPUBuffer* const>> storage_buffers
	) const noexcept
	{
		const auto color_target_info = std::to_array<SDL_GPUColorTargetInfo>({
			{.texture = target_texture,
			 .mip_level = 0,
			 .layer_or_depth_plane = 0,
			 .clear_color =
				 {.r = config.clear_color.r,
				  .g = config.clear_color.g,
				  .b = config.clear_color.b,
				  .a = config.clear_color.a},
			 .load_op = select_loadop(),
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
			[&, this](const gpu::RenderPass& render_pass) {
				base_pass.render_to_renderpass(render_pass, samplers, storage_textures, storage_buffers);
			}
		);

		return render_result;
	}
}