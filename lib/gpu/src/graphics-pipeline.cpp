#include "gpu/graphics-pipeline.hpp"
#include "gpu/util.hpp"
#include <SDL3/SDL_properties.h>

namespace gpu
{
	std::expected<GraphicsShader, util::Error> GraphicsShader::create(
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

		return GraphicsShader(device, shader);
	}

	SDL_GPUDepthStencilState GraphicsPipeline::DepthStencilState::to_sdl(
		this const DepthStencilState& self
	) noexcept
	{
		return SDL_GPUDepthStencilState{
			.compare_op = self.compare_op,
			.back_stencil_state = self.back_stencil_state,
			.front_stencil_state = self.front_stencil_state,
			.compare_mask = self.compare_mask,
			.write_mask = self.write_mask,
			.enable_depth_test = self.enable_depth_test,
			.enable_depth_write = self.enable_depth_write,
			.enable_stencil_test = self.enable_stencil_test,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};
	}

	std::expected<GraphicsPipeline, util::Error> GraphicsPipeline::create(
		SDL_GPUDevice* device,
		const GraphicsShader& vertex_shader,
		const GraphicsShader& fragment_shader,
		SDL_GPUPrimitiveType primitive_type,
		SDL_GPUSampleCount multisample_count,
		const SDL_GPURasterizerState& rasterizer_state,
		std::span<const SDL_GPUVertexAttribute> vertex_attributes,
		std::span<const SDL_GPUVertexBufferDescription> vertex_buffer_descs,
		std::span<const SDL_GPUColorTargetDescription> color_target_descs,
		const std::optional<GraphicsPipeline::DepthStencilState>& depth_stencil_state,
		const std::string& name
	) noexcept
	{
		SDL_GPUGraphicsPipelineCreateInfo create_info{};

		const auto prop = SDL_CreateProperties();
		SDL_SetStringProperty(prop, SDL_PROP_GPU_GRAPHICSPIPELINE_CREATE_NAME_STRING, name.c_str());

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
		create_info.target_info.depth_stencil_format =
			depth_stencil_state.transform([](const auto& state) { return state.format; })
				.value_or(SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT);
		create_info.target_info.has_depth_stencil_target = depth_stencil_state.has_value();

		create_info.depth_stencil_state =
			depth_stencil_state.transform([](const auto& state) { return state.to_sdl(); })
				.value_or(SDL_GPUDepthStencilState{});

		create_info.props = prop;

		auto* const raw_pipeline = SDL_CreateGPUGraphicsPipeline(device, &create_info);
		SDL_DestroyProperties(prop);
		if (raw_pipeline == nullptr) RETURN_SDL_ERROR;

		return GraphicsPipeline(device, raw_pipeline);
	}
}