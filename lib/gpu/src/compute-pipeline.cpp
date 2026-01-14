#include "gpu/compute-pipeline.hpp"
#include "gpu/util.hpp"

namespace gpu
{
	std::expected<ComputePipeline, util::Error> ComputePipeline::create(
		SDL_GPUDevice* device,
		const CreateInfo& create_info,
		const std::string& name
	) noexcept
	{
		assert(device != nullptr);
		assert(create_info.shader_data.data() != nullptr);
		assert(!create_info.shader_data.empty());

		const auto prop = SDL_CreateProperties();
		SDL_SetStringProperty(prop, SDL_PROP_GPU_COMPUTEPIPELINE_CREATE_NAME_STRING, name.c_str());

		const SDL_GPUComputePipelineCreateInfo sdl_create_info{
			.code_size = create_info.shader_data.size(),
			.code = reinterpret_cast<const uint8_t*>(create_info.shader_data.data()),
			.entrypoint = "main",
			.format = SDL_GPU_SHADERFORMAT_SPIRV,
			.num_samplers = create_info.num_samplers,
			.num_readonly_storage_textures = create_info.num_readonly_storage_textures,
			.num_readonly_storage_buffers = create_info.num_readonly_storage_buffers,
			.num_readwrite_storage_textures = create_info.num_readwrite_storage_textures,
			.num_readwrite_storage_buffers = create_info.num_readwrite_storage_buffers,
			.num_uniform_buffers = create_info.num_uniform_buffers,
			.threadcount_x = create_info.threadcount_x,
			.threadcount_y = create_info.threadcount_y,
			.threadcount_z = create_info.threadcount_z,
			.props = prop
		};

		auto* const pipeline = SDL_CreateGPUComputePipeline(device, &sdl_create_info);
		SDL_DestroyProperties(prop);
		if (pipeline == nullptr) RETURN_SDL_ERROR;

		return ComputePipeline(device, pipeline);
	}
}