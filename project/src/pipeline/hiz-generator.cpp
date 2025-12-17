#include "pipeline/hiz-generator.hpp"

#include "asset/shader/hiz-gen.comp.hpp"
#include "gpu/compute-pipeline.hpp"
#include "target/gbuffer.hpp"
#include "util/as-byte.hpp"

#include <SDL3/SDL_gpu.h>
#include <format>
#include <ranges>

namespace pipeline
{
	std::expected<Hiz_generator, util::Error> Hiz_generator::create(SDL_GPUDevice* device) noexcept
	{
		const gpu::Compute_pipeline::Create_info create_info{
			.shader_data = shader_asset::hiz_gen_comp,
			.num_samplers = 0,
			.num_readonly_storage_textures = 0,
			.num_readwrite_storage_textures = 2,
			.num_readonly_storage_buffers = 0,
			.num_readwrite_storage_buffers = 0,
			.num_uniform_buffers = 1,
			.threadcount_x = 16,
			.threadcount_y = 16,
			.threadcount_z = 1
		};

		auto pipeline_result = gpu::Compute_pipeline::create(device, create_info, "HiZ Generator Pipeline");
		if (!pipeline_result)
			return pipeline_result.error().forward("Create pipeline for HiZ generator failed");

		return Hiz_generator(std::move(pipeline_result.value()));
	}

	std::expected<void, util::Error> Hiz_generator::generate(
		const gpu::Command_buffer& command_buffer,
		const target::Gbuffer& gbuffer,
		glm::u32vec2 hiz_top_size
	) const noexcept
	{
		glm::u32vec2 current_src_size = hiz_top_size;

		command_buffer.push_debug_group("Hi-Z Generation");
		for (const auto src_mip : std::views::iota(0u, target::Gbuffer::hiz_mip_levels - 1))
		{
			const auto dst_mip = src_mip + 1;

			const auto src_binding = SDL_GPUStorageTextureReadWriteBinding{
				.texture = gbuffer.depth_value_texture.current(),
				.mip_level = src_mip,
				.layer = 0,
				.cycle = false,
				.padding1 = 0,
				.padding2 = 0,
				.padding3 = 0
			};

			const auto dst_binding = SDL_GPUStorageTextureReadWriteBinding{
				.texture = gbuffer.depth_value_texture.current(),
				.mip_level = dst_mip,
				.layer = 0,
				.cycle = false,
				.padding1 = 0,
				.padding2 = 0,
				.padding3 = 0
			};

			Param param{
				.src_size = glm::i32vec2(current_src_size),
				.dst_size = glm::i32vec2(current_src_size / 2u),
			};

			command_buffer.push_uniform_to_compute(0, util::as_bytes(param));

			const auto workgroup_count = (param.dst_size + 15) / 16;

			const auto result = command_buffer.run_compute_pass(
				std::to_array({src_binding, dst_binding}),
				{},
				[this, workgroup_count](const gpu::Compute_pass& pass) {
					pass.bind_pipeline(this->pipeline);
					pass.dispatch(workgroup_count.x, workgroup_count.y, 1);
				}
			);
			if (!result)
				return result.error().forward(std::format("Hiz generation failed at src_mip {}", src_mip));

			current_src_size /= 2u;
		}
		command_buffer.pop_debug_group();

		return {};
	}
}