#include "pipeline/bloom.hpp"

#include "asset/shader/bloom-add.comp.hpp"
#include "asset/shader/bloom-blur.comp.hpp"
#include "asset/shader/bloom-filter.comp.hpp"
#include "gpu/compute-pass.hpp"
#include "gpu/compute-pipeline.hpp"
#include "gpu/sampler.hpp"
#include "target/bloom.hpp"
#include "util/as-byte.hpp"
#include <SDL3/SDL_gpu.h>
#include <ranges>

namespace pipeline
{
	static glm::u32vec2 calculate_mip_size(glm::u32vec2 in_size, uint32_t mip) noexcept
	{
		for (auto _ : std::views::iota(0u, mip)) in_size /= 2u;
		return in_size;
	}

	std::expected<Bloom, util::Error> Bloom::create(SDL_GPUDevice* device) noexcept
	{
		const auto add_shader_code = shader_asset::bloom_add_comp;
		const auto blur_shader_code = shader_asset::bloom_blur_comp;
		const auto filter_shader_code = shader_asset::bloom_filter_comp;

		const auto add_pipeline_config = gpu::Compute_pipeline::Create_info{
			.shader_data = add_shader_code,
			.num_samplers = 2,
			.num_readonly_storage_textures = 0,
			.num_readwrite_storage_textures = 1,
			.num_readonly_storage_buffers = 0,
			.num_readwrite_storage_buffers = 0,
			.num_uniform_buffers = 1,
			.threadcount_x = 16,
			.threadcount_y = 16,
			.threadcount_z = 1
		};

		const auto blur_pipeline_config = gpu::Compute_pipeline::Create_info{
			.shader_data = blur_shader_code,
			.num_samplers = 1,
			.num_readonly_storage_textures = 0,
			.num_readwrite_storage_textures = 1,
			.num_readonly_storage_buffers = 0,
			.num_readwrite_storage_buffers = 0,
			.num_uniform_buffers = 0,
			.threadcount_x = 16,
			.threadcount_y = 16,
			.threadcount_z = 1
		};

		const auto filter_pipeline_config = gpu::Compute_pipeline::Create_info{
			.shader_data = filter_shader_code,
			.num_samplers = 0,
			.num_readonly_storage_textures = 1,
			.num_readwrite_storage_textures = 1,
			.num_readonly_storage_buffers = 1,
			.num_readwrite_storage_buffers = 0,
			.num_uniform_buffers = 1,
			.threadcount_x = 16,
			.threadcount_y = 16,
			.threadcount_z = 1
		};

		const auto sampler_create_config = gpu::Sampler::Create_info{
			.min_filter = gpu::Sampler::Filter::Linear,
			.mag_filter = gpu::Sampler::Filter::Linear,
			.mipmap_mode = gpu::Sampler::Mipmap_mode::Nearest,
			.address_mode_u = gpu::Sampler::Address_mode::Clamp_to_edge,
			.address_mode_v = gpu::Sampler::Address_mode::Clamp_to_edge,
			.address_mode_w = gpu::Sampler::Address_mode::Clamp_to_edge,
			.min_lod = 0,
			.max_lod = 0
		};

		auto filter_pipeline =
			gpu::Compute_pipeline::create(device, filter_pipeline_config, "Bloom filter pipeline");
		auto blur_pipeline =
			gpu::Compute_pipeline::create(device, blur_pipeline_config, "Bloom blur pipeline");
		auto add_pipeline = gpu::Compute_pipeline::create(device, add_pipeline_config, "Bloom add pipeline");
		auto linear_sampler = gpu::Sampler::create(device, sampler_create_config);

		if (!filter_pipeline) return filter_pipeline.error().forward("Create bloom filter pipeline failed");
		if (!blur_pipeline) return blur_pipeline.error().forward("Create bloom blur pipeline failed");
		if (!add_pipeline) return add_pipeline.error().forward("Create bloom add pipeline failed");
		if (!linear_sampler) return linear_sampler.error().forward("Create bloom linear sampler failed");

		return Bloom(
			std::move(*filter_pipeline),
			std::move(*blur_pipeline),
			std::move(*add_pipeline),
			std::move(*linear_sampler)
		);
	}

	std::expected<void, util::Error> Bloom::render(
		const gpu::Command_buffer& command_buffer,
		const target::Light_buffer& light_buffer_target,
		const target::Bloom& bloom_target,
		const target::Auto_exposure& auto_exposure_target,
		const Param& param,
		glm::u32vec2 swapchain_size
	) const noexcept
	{
		command_buffer.push_debug_group("Bloom pass");

		command_buffer.push_debug_group("Filter pass");
		{
			const auto filter_pass_result = run_filter_pass(
				command_buffer,
				light_buffer_target.light_texture.current(),
				bloom_target.get_filter_texture(),
				auto_exposure_target.get_current_frame().result_buffer,
				param,
				swapchain_size
			);
			if (!filter_pass_result) return filter_pass_result.error().forward("Bloom filter pass failed");
		}
		command_buffer.pop_debug_group();

		command_buffer.push_debug_group("Initial blit");
		{
			run_initial_blit(command_buffer, bloom_target, swapchain_size);
		}
		command_buffer.pop_debug_group();

		command_buffer.push_debug_group("Blur and blit passes");
		for (const auto upsample_mipmap_level : std::views::iota(0u, target::Bloom::upsample_mip_count))
		{
			const auto blur_pass_result =
				run_blur_pass(command_buffer, bloom_target, upsample_mipmap_level, swapchain_size);
			if (!blur_pass_result) return blur_pass_result.error().forward("Bloom blur pass failed");

			run_blit(command_buffer, bloom_target, upsample_mipmap_level, swapchain_size);
		}
		command_buffer.pop_debug_group();

		command_buffer.push_debug_group("Final add passes");
		for (const auto upsample_mipmap_level :
			 std::views::iota(0u, target::Bloom::upsample_mip_count) | std::views::reverse)
		{
			const auto add_pass_result =
				run_add_pass(command_buffer, bloom_target, upsample_mipmap_level, param, swapchain_size);
			if (!add_pass_result) return add_pass_result.error().forward("Bloom add pass failed");
		}
		command_buffer.pop_debug_group();

		command_buffer.pop_debug_group();

		return {};
	}

	std::expected<void, util::Error> Bloom::run_filter_pass(
		const gpu::Command_buffer& command_buffer,
		SDL_GPUTexture* input_texture,
		SDL_GPUTexture* output_texture,
		SDL_GPUBuffer* exposure_buffer,
		const Param& param,
		glm::u32vec2 swapchain_size
	) const noexcept
	{
		const auto output_binding = SDL_GPUStorageTextureReadWriteBinding{
			.texture = output_texture,
			.mip_level = 0,
			.layer = 0,
			.cycle = true,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		const auto workgroup_count = (swapchain_size + 15u) / 16u;

		command_buffer.push_uniform_to_compute(0, util::as_bytes(param));

		return command_buffer.run_compute_pass(
			std::to_array({output_binding}),
			{},
			[this, input_texture, exposure_buffer, workgroup_count](const gpu::Compute_pass& pass) {
				pass.bind_pipeline(filter);
				pass.bind_storage_textures(0, input_texture);
				pass.bind_storage_buffers(0, exposure_buffer);
				pass.dispatch(workgroup_count.x, workgroup_count.y, 1);
			}
		);
	}

	std::expected<void, util::Error> Bloom::run_blur_pass(
		const gpu::Command_buffer& command_buffer,
		const target::Bloom& bloom_target,
		uint32_t upsample_mip_level,
		glm::u32vec2 swapchain_size
	) const noexcept
	{
		const auto input_binding =
			bloom_target.get_downsample_chain(upsample_mip_level).bind_with_sampler(linear_sampler);

		const SDL_GPUStorageTextureReadWriteBinding output_binding = {
			.texture = bloom_target.get_upsample_chain(upsample_mip_level),
			.mip_level = 0,
			.layer = 0,
			.cycle = upsample_mip_level == 0,  // Cycle only when blurring the first mip level
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		const auto image_size = calculate_mip_size(swapchain_size, upsample_mip_level + 1);

		const auto workgroup_count = (image_size + 15u) / 16u;

		return command_buffer.run_compute_pass(
			std::to_array({output_binding}),
			{},
			[this, workgroup_count, input_binding](const gpu::Compute_pass& pass) {
				pass.bind_pipeline(blur);
				pass.bind_samplers(0, input_binding);
				pass.dispatch(workgroup_count.x, workgroup_count.y, 1);
			}
		);
	}

	void Bloom::run_blit(
		const gpu::Command_buffer& command_buffer,
		const target::Bloom& bloom_target,
		uint32_t upsample_mip_level,
		glm::u32vec2 swapchain_size
	) const noexcept
	{
		const auto src_size = calculate_mip_size(swapchain_size, upsample_mip_level + 1);
		const auto dst_size = src_size / 2u;

		const SDL_GPUBlitRegion src_region = {
			.texture = bloom_target.get_upsample_chain(upsample_mip_level),
			.mip_level = 0,
			.layer_or_depth_plane = 0,
			.x = 0,
			.y = 0,
			.w = src_size.x,
			.h = src_size.y
		};

		const SDL_GPUBlitRegion dst_region = {
			.texture = bloom_target.get_downsample_chain(upsample_mip_level + 1),
			.mip_level = 0,
			.layer_or_depth_plane = 0,
			.x = 0,
			.y = 0,
			.w = dst_size.x,
			.h = dst_size.y
		};

		const SDL_GPUBlitInfo blit_info = {
			.source = src_region,
			.destination = dst_region,
			.load_op = SDL_GPU_LOADOP_DONT_CARE,
			.clear_color = {},
			.flip_mode = SDL_FLIP_NONE,
			.filter = SDL_GPU_FILTER_LINEAR,
			.cycle = false,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		command_buffer.blit_texture(blit_info);
	}

	void Bloom::run_initial_blit(
		const gpu::Command_buffer& command_buffer,
		const target::Bloom& bloom_target,
		glm::u32vec2 swapchain_size
	) const noexcept
	{
		const auto src_size = swapchain_size;
		const auto dst_size = src_size / 2u;

		const SDL_GPUBlitRegion src_region = {
			.texture = bloom_target.get_filter_texture(),
			.mip_level = 0,
			.layer_or_depth_plane = 0,
			.x = 0,
			.y = 0,
			.w = src_size.x,
			.h = src_size.y
		};

		const SDL_GPUBlitRegion dst_region = {
			.texture = bloom_target.get_downsample_chain(0),
			.mip_level = 0,
			.layer_or_depth_plane = 0,
			.x = 0,
			.y = 0,
			.w = dst_size.x,
			.h = dst_size.y
		};

		const SDL_GPUBlitInfo blit_info = {
			.source = src_region,
			.destination = dst_region,
			.load_op = SDL_GPU_LOADOP_DONT_CARE,
			.clear_color = {},
			.flip_mode = SDL_FLIP_NONE,
			.filter = SDL_GPU_FILTER_LINEAR,
			.cycle = true,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		command_buffer.blit_texture(blit_info);
	}

	std::expected<void, util::Error> Bloom::run_add_pass(
		const gpu::Command_buffer& command_buffer,
		const target::Bloom& bloom_target,
		uint32_t upsample_mip_level,
		const Param& param,
		glm::u32vec2 swapchain_size
	) const noexcept
	{
		const auto upsample_dst_size = calculate_mip_size(swapchain_size, upsample_mip_level + 1);
		const bool is_last_mip = (upsample_mip_level + 1) == target::Bloom::upsample_mip_count;

		const Add_param add_param(param);
		command_buffer.push_uniform_to_compute(0, util::as_bytes(add_param));

		const auto prev_upsample_binding =
			(is_last_mip ? bloom_target.get_downsample_chain(target::Bloom::downsample_mip_count - 1)
						 : bloom_target.get_upsample_chain(upsample_mip_level + 1))
				.bind_with_sampler(linear_sampler);

		const auto cur_downsample_binding =
			bloom_target.get_downsample_chain(upsample_mip_level).bind_with_sampler(linear_sampler);

		const SDL_GPUStorageTextureReadWriteBinding cur_upsample_binding = {
			.texture = bloom_target.get_upsample_chain(upsample_mip_level),
			.mip_level = 0,
			.layer = 0,
			.cycle = is_last_mip,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		const auto workgroup_count = (upsample_dst_size + 15u) / 16u;

		return command_buffer.run_compute_pass(
			std::to_array({cur_upsample_binding}),
			{},
			[this, workgroup_count, prev_upsample_binding, cur_downsample_binding](
				const gpu::Compute_pass& pass
			) {
				pass.bind_pipeline(blur_and_add);
				pass.bind_samplers(0, prev_upsample_binding, cur_downsample_binding);
				pass.dispatch(workgroup_count.x, workgroup_count.y, 1);
			}
		);
	}
}