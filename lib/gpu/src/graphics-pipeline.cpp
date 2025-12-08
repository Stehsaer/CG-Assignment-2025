#include "gpu/graphics-pipeline.hpp"
#include "gpu/util.hpp"

namespace gpu
{
	std::expected<Graphic_shader, util::Error> Graphic_shader::create(
		SDL_GPUDevice* device,
		std::span<const std::byte> shader_data,
		Stage stage,
		uint32_t num_samplers,
		uint32_t num_storage_textures,
		uint32_t num_storage_buffers,
		uint32_t num_uniform_buffers
	) noexcept
	{
		assert(device != nullptr);
		assert(shader_data.data() != nullptr);
		assert(!shader_data.empty());

		const SDL_GPUShaderCreateInfo info{
			.code_size = shader_data.size(),
			.code = reinterpret_cast<const uint8_t*>(shader_data.data()),
			.entrypoint = "main",
			.format = SDL_GPU_SHADERFORMAT_SPIRV,
			.stage = static_cast<SDL_GPUShaderStage>(stage),
			.num_samplers = num_samplers,
			.num_storage_textures = num_storage_textures,
			.num_storage_buffers = num_storage_buffers,
			.num_uniform_buffers = num_uniform_buffers,
			.props = 0
		};

		SDL_GPUShader* const shader = SDL_CreateGPUShader(device, &info);
		if (shader == nullptr) RETURN_SDL_ERROR;

		return Graphic_shader(device, shader);
	}

	std::expected<Graphics_pipeline, util::Error> Graphics_pipeline::create(
		SDL_GPUDevice* device,
		const Graphic_shader& vertex_shader,
		const Graphic_shader& fragment_shader,
		SDL_GPUPrimitiveType primitive_type,
		SDL_GPUSampleCount multisample_count,
		const SDL_GPURasterizerState& rasterizer_state,
		std::span<const SDL_GPUVertexAttribute> vertex_attributes,
		std::span<const SDL_GPUVertexBufferDescription> vertex_buffer_descs,
		std::span<const SDL_GPUColorTargetDescription> color_target_descs,
		const std::optional<Graphics_pipeline::Depth_stencil_state>& depth_stencil_state
	) noexcept
	{
		SDL_GPUGraphicsPipelineCreateInfo create_info{};

		create_info.vertex_shader = vertex_shader;
		create_info.fragment_shader = fragment_shader;

		create_info.vertex_input_state = SDL_GPUVertexInputState{
			.vertex_buffer_descriptions = vertex_buffer_descs.data(),
			.num_vertex_buffers = static_cast<Uint32>(vertex_buffer_descs.size()),
			.vertex_attributes = vertex_attributes.data(),
			.num_vertex_attributes = static_cast<Uint32>(vertex_attributes.size()),
		};

		create_info.primitive_type = primitive_type;
		create_info.rasterizer_state = rasterizer_state;
		create_info.multisample_state.sample_count = multisample_count;

		create_info.target_info.color_target_descriptions = color_target_descs.data();
		create_info.target_info.num_color_targets = static_cast<Uint32>(color_target_descs.size());
		create_info.target_info.depth_stencil_format = depth_stencil_state.has_value()
			? depth_stencil_state->format
			: SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
		create_info.target_info.has_depth_stencil_target = depth_stencil_state.has_value();

		if (depth_stencil_state.has_value())
			create_info.depth_stencil_state = SDL_GPUDepthStencilState{
				.compare_op = depth_stencil_state->compare_op,
				.back_stencil_state = depth_stencil_state->back_stencil_state,
				.front_stencil_state = depth_stencil_state->front_stencil_state,
				.compare_mask = depth_stencil_state->compare_mask,
				.write_mask = depth_stencil_state->write_mask,
				.enable_depth_test = depth_stencil_state->enable_depth_test,
				.enable_depth_write = depth_stencil_state->enable_depth_write,
				.enable_stencil_test = depth_stencil_state->enable_stencil_test,
				.padding1 = 0,
				.padding2 = 0,
				.padding3 = 0
			};
		else
		{
			create_info.depth_stencil_state = SDL_GPUDepthStencilState{};
		}

		auto* const raw_pipeline = SDL_CreateGPUGraphicsPipeline(device, &create_info);
		if (raw_pipeline == nullptr) RETURN_SDL_ERROR;

		return Graphics_pipeline(device, raw_pipeline);
	}
}