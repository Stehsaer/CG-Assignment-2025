#include "pipeline/ssgi.hpp"
#include "asset/graphic-asset.hpp"
#include "asset/shader/init-temporal.comp.hpp"
#include "asset/shader/radiance-add.frag.hpp"
#include "asset/shader/radiance-composite.comp.hpp"
#include "asset/shader/radiance-upsample.comp.hpp"
#include "asset/shader/spatial-reuse.comp.hpp"
#include "gpu/compute-pass.hpp"
#include "gpu/compute-pipeline.hpp"
#include "gpu/graphics-pipeline.hpp"
#include "graphics/util/fullscreen-pass.hpp"
#include "graphics/util/quick-create.hpp"
#include "image/io.hpp"
#include "target/light.hpp"
#include "util/as-byte.hpp"
#include "util/asset.hpp"
#include "zip/zip.hpp"

#include <SDL3/SDL_gpu.h>
#include <random>

namespace pipeline
{
	SSGI::Initial_temporal_param SSGI::Initial_temporal_param::from_param(
		const Param& param,
		const glm::uvec2& resolution
	) noexcept
	{
		static thread_local std::mt19937 generator{std::random_device{}()};
		static thread_local std::uniform_int_distribution<int> distribution{0, 128};

		const glm::mat4 inv_proj_mat = glm::inverse(param.proj_mat);

		const glm::vec4 near_plane_center_clip = {0.0, 0.0, 1.0, 1.0};
		const glm::vec4 near_plane_corner_clip = {1.0, 1.0, 1.0, 1.0};
		glm::vec4 near_plane_center_view = inv_proj_mat * near_plane_center_clip;
		glm::vec4 near_plane_corner_view = inv_proj_mat * near_plane_corner_clip;
		near_plane_center_view /= near_plane_center_view.w;
		near_plane_corner_view /= near_plane_corner_view.w;

		const glm::vec2 near_plane_span = {
			near_plane_corner_view.x - near_plane_center_view.x,
			near_plane_corner_view.y - near_plane_center_view.y
		};

		const glm::ivec2 time_noise = {distribution(generator), distribution(generator)};

		return Initial_temporal_param{
			.inv_proj_mat = inv_proj_mat,
			.proj_mat = param.proj_mat,
			.inv_view_mat = glm::inverse(param.view_mat),
			.view_mat = param.view_mat,
			.back_proj_mat = param.prev_view_proj_mat * glm::inverse(param.view_mat),
			.inv_proj_mat_col3 = inv_proj_mat[2],
			.inv_proj_mat_col4 = inv_proj_mat[3],
			.resolution = resolution,
			.time_noise = time_noise,
			.near_plane_span = near_plane_span,
			.near_plane = -near_plane_center_view.z,
			.max_scene_distance = param.max_scene_distance,
			.distance_attenuation = param.distance_attenuation
		};
	}

	SSGI::Spatial_reuse_param SSGI::Spatial_reuse_param::from_param(
		const Param& param,
		const glm::uvec2& resolution
	) noexcept
	{
		static thread_local std::mt19937 generator{std::random_device{}()};
		static thread_local std::uniform_int_distribution<int> distribution{0, 128};

		const glm::ivec2 time_noise = {distribution(generator), distribution(generator)};

		const glm::mat4 inv_proj_mat = glm::inverse(param.proj_mat);

		const glm::vec4 near_plane_center_clip = {0.0, 0.0, 1.0, 1.0};
		const glm::vec4 near_plane_corner_clip = {1.0, 1.0, 1.0, 1.0};
		glm::vec4 near_plane_center_view = inv_proj_mat * near_plane_center_clip;
		glm::vec4 near_plane_corner_view = inv_proj_mat * near_plane_corner_clip;
		near_plane_center_view /= near_plane_center_view.w;
		near_plane_corner_view /= near_plane_corner_view.w;

		const glm::vec2 near_plane_span = {
			near_plane_corner_view.x - near_plane_center_view.x,
			near_plane_corner_view.y - near_plane_center_view.y
		};

		return Spatial_reuse_param{
			.inv_view_proj_mat = glm::inverse(param.proj_mat * param.view_mat),
			.prev_view_proj_mat = param.prev_view_proj_mat,
			.proj_mat = param.proj_mat,
			.view_mat = param.view_mat,
			.inv_view_mat = glm::inverse(param.view_mat),
			.inv_proj_mat_col3 = inv_proj_mat[2],
			.inv_proj_mat_col4 = inv_proj_mat[3],
			.comp_resolution = (resolution + 1u) / 2u,
			.full_resolution = resolution,
			.near_plane_span = near_plane_span,
			.near_plane = -near_plane_center_view.z,
			.time_noise = time_noise
		};
	}

	SSGI::Radiance_composite_param SSGI::Radiance_composite_param::from_param(
		const Param& param,
		glm::u32vec2 resolution
	) noexcept
	{
		return Radiance_composite_param{
			.back_projection_mat = param.prev_view_proj_mat,
			.inv_back_projection_mat = glm::inverse(param.prev_view_proj_mat),
			.inv_view_proj_mat = glm::inverse(param.proj_mat * param.view_mat),
			.inv_view_mat = glm::inverse(param.view_mat),
			.comp_resolution = (resolution + 1u) / 2u,
			.full_resolution = resolution,
			.blend_factor = param.blend_factor
		};
	}

	SSGI::Radiance_upsample_param SSGI::Radiance_upsample_param::from_param(
		const Param& param,
		glm::u32vec2 resolution
	) noexcept
	{
		return Radiance_upsample_param{
			.inv_view_proj_mat_col3 = glm::inverse(param.proj_mat * param.view_mat)[2],
			.inv_view_proj_mat_col4 = glm::inverse(param.proj_mat * param.view_mat)[3],
			.comp_resolution = (resolution + 1u) / 2u,
			.full_resolution = resolution
		};
	}

	std::expected<SSGI, util::Error> SSGI::create(SDL_GPUDevice* device) noexcept
	{
		/* Read Image */

		const auto noise_image_name = "blue-noise.png";
		auto noise_texture =
			util::get_asset(resource_asset::graphic_asset, noise_image_name)
				.and_then(zip::Decompress())
				.and_then(image::load_from_memory<image::Precision::U16, image::Format::RGBA>)
				.and_then([device](const auto& image) {
					return graphics::create_texture_from_image(
						device,
						gpu::Texture::Format{
							.type = SDL_GPU_TEXTURETYPE_2D,
							.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_UNORM,
							.usage = {.sampler = true}
						},
						image,
						"SSGI Blue Noise Texture"
					);
				});
		if (!noise_texture) return noise_texture.error().forward("Create SSGI blue noise texture failed");

		/* Create Samplers */

		const auto linear_sampler_create_info = gpu::Sampler::Create_info{
			.min_filter = gpu::Sampler::Filter::Linear,
			.mag_filter = gpu::Sampler::Filter::Linear,
			.mipmap_mode = gpu::Sampler::Mipmap_mode::Nearest,
			.address_mode_u = gpu::Sampler::Address_mode::Repeat,
			.address_mode_v = gpu::Sampler::Address_mode::Repeat,
			.address_mode_w = gpu::Sampler::Address_mode::Repeat
		};

		const auto nearest_sampler_create_info = gpu::Sampler::Create_info{
			.min_filter = gpu::Sampler::Filter::Nearest,
			.mag_filter = gpu::Sampler::Filter::Nearest,
			.mipmap_mode = gpu::Sampler::Mipmap_mode::Nearest,
			.address_mode_u = gpu::Sampler::Address_mode::Clamp_to_edge,
			.address_mode_v = gpu::Sampler::Address_mode::Clamp_to_edge,
			.address_mode_w = gpu::Sampler::Address_mode::Clamp_to_edge,
			.min_lod = 0,
			.max_lod = 10
		};

		const auto noise_sampler_create_info = gpu::Sampler::Create_info{
			.min_filter = gpu::Sampler::Filter::Nearest,
			.mag_filter = gpu::Sampler::Filter::Nearest,
			.mipmap_mode = gpu::Sampler::Mipmap_mode::Nearest,
			.address_mode_u = gpu::Sampler::Address_mode::Repeat,
			.address_mode_v = gpu::Sampler::Address_mode::Repeat,
			.address_mode_w = gpu::Sampler::Address_mode::Repeat
		};

		auto nearest_sampler = gpu::Sampler::create(device, nearest_sampler_create_info);
		auto noise_sampler = gpu::Sampler::create(device, noise_sampler_create_info);
		auto linear_sampler = gpu::Sampler::create(device, linear_sampler_create_info);
		if (!nearest_sampler) return nearest_sampler.error().forward("Create SSGI nearest sampler failed");
		if (!noise_sampler) return noise_sampler.error().forward("Create SSGI noise sampler failed");
		if (!linear_sampler) return linear_sampler.error().forward("Create SSGI linear sampler failed");

		/* Create Pipeline */

		const gpu::Compute_pipeline::Create_info initial_pipeline_create_info{
			.shader_data = shader_asset::init_temporal_comp,
			.num_samplers = 9,
			.num_readwrite_storage_textures = 4,
			.num_uniform_buffers = 1,
			.threadcount_x = 8,
			.threadcount_y = 8,
			.threadcount_z = 1
		};

		const gpu::Compute_pipeline::Create_info spatial_reuse_pipeline_create_info{
			.shader_data = shader_asset::spatial_reuse_comp,
			.num_samplers = 13,
			.num_readwrite_storage_textures = 4,
			.num_uniform_buffers = 1,
			.threadcount_x = 8,
			.threadcount_y = 8,
			.threadcount_z = 1
		};

		auto initial_pipeline =
			gpu::Compute_pipeline::create(device, initial_pipeline_create_info, "SSGI Trace Pipeline");
		if (!initial_pipeline) return initial_pipeline.error().forward("Create SSGI pipeline failed");

		auto spatial_reuse_pipeline = gpu::Compute_pipeline::create(
			device,
			spatial_reuse_pipeline_create_info,
			"SSGI Spatial Reuse Pipeline"
		);
		if (!spatial_reuse_pipeline)
			return spatial_reuse_pipeline.error().forward("Create SSGI spatial reuse pipeline failed");

		auto radiance_composite_pipeline = gpu::Compute_pipeline::create(
			device,
			gpu::Compute_pipeline::Create_info{
				.shader_data = shader_asset::radiance_composite_comp,
				.num_samplers = 9,
				.num_readwrite_storage_textures = 1,
				.num_uniform_buffers = 1,
				.threadcount_x = 16,
				.threadcount_y = 16,
				.threadcount_z = 1
			},
			"SSGI Radiance Composite Pipeline"
		);
		if (!radiance_composite_pipeline)
			return radiance_composite_pipeline.error().forward(
				"Create SSGI radiance composite pipeline failed"
			);

		auto radiance_upsample_pipeline = gpu::Compute_pipeline::create(
			device,
			gpu::Compute_pipeline::Create_info{
				.shader_data = shader_asset::radiance_upsample_comp,
				.num_samplers = 4,
				.num_readwrite_storage_textures = 1,
				.num_uniform_buffers = 1,
				.threadcount_x = 16,
				.threadcount_y = 16,
				.threadcount_z = 1
			},
			"SSGI Radiance Upsample Pipeline"
		);
		if (!radiance_upsample_pipeline)
			return radiance_upsample_pipeline.error().forward(
				"Create SSGI radiance upsample pipeline failed"
			);

		const auto radiance_add_shader = gpu::Graphics_shader::create(
			device,
			shader_asset::radiance_add_frag,
			gpu::Graphics_shader::Stage::Fragment,
			1,
			0,
			0,
			0
		);
		if (!radiance_add_shader)
			return radiance_add_shader.error().forward("Create SSGI radiance add shader failed");

		auto radiance_add_pass = graphics::Fullscreen_pass<true>::create(
			device,
			*radiance_add_shader,
			target::Light_buffer::light_buffer_format,
			{.clear_before_render = false,
			 .do_cycle = false,
			 .blend_mode = graphics::Fullscreen_blend_mode::Add},
			"SSGI Radiance Add Pipeline"
		);
		if (!radiance_add_pass)
			return radiance_add_pass.error().forward("Create SSGI radiance add pass failed");

		return SSGI(
			std::move(*initial_pipeline),
			std::move(*spatial_reuse_pipeline),
			std::move(*radiance_composite_pipeline),
			std::move(*radiance_upsample_pipeline),
			std::move(*radiance_add_pass),
			std::move(*noise_sampler),
			std::move(*nearest_sampler),
			std::move(*linear_sampler),
			std::move(*noise_texture)
		);
	}

	std::expected<void, util::Error> SSGI::render(
		const gpu::Command_buffer& command_buffer,
		const target::Light_buffer& light_buffer,
		const target::Gbuffer& gbuffer,
		const target::SSGI& ssgi_target,
		const Param& param,
		glm::u32vec2 resolution
	) const noexcept
	{
		command_buffer.push_debug_group("SSGI");

		const auto initial_sample_result =
			this->run_initial_sample(command_buffer, light_buffer, gbuffer, ssgi_target, param, resolution);
		if (!initial_sample_result)
			return initial_sample_result.error().forward("Run SSGI initial sample failed");

		const auto spatial_reuse_result =
			this->run_spatial_reuse(command_buffer, gbuffer, ssgi_target, param, resolution);
		if (!spatial_reuse_result)
			return spatial_reuse_result.error().forward("Run SSGI spatial reuse failed");

		const auto radiance_composite_result =
			this->run_radiance_composite(command_buffer, gbuffer, ssgi_target, param, resolution);
		if (!radiance_composite_result)
			return radiance_composite_result.error().forward("Run SSGI radiance composite failed");

		const auto radiance_add_result =
			this->render_radiance_add(command_buffer, light_buffer, ssgi_target, resolution);
		if (!radiance_add_result) return radiance_add_result.error().forward("Run SSGI radiance add failed");

		const auto radiance_upsample_result =
			this->run_radiance_upsample(command_buffer, gbuffer, ssgi_target, param, resolution);
		if (!radiance_upsample_result)
			return radiance_upsample_result.error().forward("Run SSGI radiance upsample failed");

		command_buffer.pop_debug_group();

		return {};
	}

	std::expected<void, util::Error> SSGI::run_initial_sample(
		const gpu::Command_buffer& command_buffer,
		const target::Light_buffer& light_buffer,
		const target::Gbuffer& gbuffer,
		const target::SSGI& ssgi_target,
		const Param& param,
		glm::u32vec2 resolution
	) const noexcept
	{
		const auto half_res = (resolution + 1u) / 2u;
		const auto dispatch_size = (half_res + 7u) / 8u;

		const Initial_temporal_param initial_sample_param =
			Initial_temporal_param::from_param(param, resolution);
		command_buffer.push_uniform_to_compute(0, util::as_bytes(initial_sample_param));

		const SDL_GPUStorageTextureReadWriteBinding write_binding_texture1{
			.texture = ssgi_target.temporal_reservoir_texture1.current(),
			.mip_level = 0,
			.layer = 0,
			.cycle = true,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		const SDL_GPUStorageTextureReadWriteBinding write_binding_texture2{
			.texture = ssgi_target.temporal_reservoir_texture2.current(),
			.mip_level = 0,
			.layer = 0,
			.cycle = true,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		const SDL_GPUStorageTextureReadWriteBinding write_binding_texture3{
			.texture = ssgi_target.temporal_reservoir_texture3.current(),
			.mip_level = 0,
			.layer = 0,
			.cycle = true,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		const SDL_GPUStorageTextureReadWriteBinding write_binding_texture4{
			.texture = ssgi_target.temporal_reservoir_texture4.current(),
			.mip_level = 0,
			.layer = 0,
			.cycle = true,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		const std::array write_bindings =
			{write_binding_texture1, write_binding_texture2, write_binding_texture3, write_binding_texture4};

		command_buffer.push_debug_group("Initial Sample & Temporal Pass");
		auto result = command_buffer.run_compute_pass(
			write_bindings,
			{},
			[this, &light_buffer, &gbuffer, &ssgi_target, dispatch_size](
				const gpu::Compute_pass& compute_pass
			) {
				compute_pass.bind_pipeline(initial_pipeline);
				compute_pass.bind_samplers(
					0,
					light_buffer.light_texture.current().bind_with_sampler(nearest_sampler),
					gbuffer.depth_value_texture.current().bind_with_sampler(nearest_sampler),
					gbuffer.lighting_info_texture->bind_with_sampler(nearest_sampler),
					noise_texture.bind_with_sampler(noise_sampler),
					gbuffer.depth_value_texture.prev().bind_with_sampler(linear_sampler),

					ssgi_target.temporal_reservoir_texture1.prev().bind_with_sampler(nearest_sampler),
					ssgi_target.temporal_reservoir_texture2.prev().bind_with_sampler(nearest_sampler),
					ssgi_target.temporal_reservoir_texture3.prev().bind_with_sampler(nearest_sampler),
					ssgi_target.temporal_reservoir_texture4.prev().bind_with_sampler(nearest_sampler)
				);
				compute_pass.dispatch(dispatch_size.x, dispatch_size.y, 1);
			}
		);
		command_buffer.pop_debug_group();

		return std::move(result).transform_error(
			util::Error::forward_fn("Initial Sample & Temporal Pass failed")
		);
	}

	std::expected<void, util::Error> SSGI::run_spatial_reuse(
		const gpu::Command_buffer& command_buffer,
		const target::Gbuffer& gbuffer,
		const target::SSGI& ssgi_target,
		const Param& param,
		glm::u32vec2 resolution
	) const noexcept
	{
		const Spatial_reuse_param spatial_reuse_param = Spatial_reuse_param::from_param(param, resolution);
		command_buffer.push_uniform_to_compute(0, util::as_bytes(spatial_reuse_param));

		const auto dispatch_size = (spatial_reuse_param.comp_resolution + 7u) / 8u;

		const SDL_GPUStorageTextureReadWriteBinding write_binding_texture1{
			.texture = ssgi_target.spatial_reservoir_texture1.current(),
			.mip_level = 0,
			.layer = 0,
			.cycle = true,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		const SDL_GPUStorageTextureReadWriteBinding write_binding_texture2{
			.texture = ssgi_target.spatial_reservoir_texture2.current(),
			.mip_level = 0,
			.layer = 0,
			.cycle = true,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		const SDL_GPUStorageTextureReadWriteBinding write_binding_texture3{
			.texture = ssgi_target.spatial_reservoir_texture3.current(),
			.mip_level = 0,
			.layer = 0,
			.cycle = true,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		const SDL_GPUStorageTextureReadWriteBinding write_binding_texture4{
			.texture = ssgi_target.spatial_reservoir_texture4.current(),
			.mip_level = 0,
			.layer = 0,
			.cycle = true,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		const std::array write_bindings =
			{write_binding_texture1, write_binding_texture2, write_binding_texture3, write_binding_texture4};

		command_buffer.push_debug_group("Spatial Reuse Pass");
		auto result = command_buffer.run_compute_pass(
			write_bindings,
			{},
			[this, &gbuffer, &ssgi_target, dispatch_size](const gpu::Compute_pass& compute_pass) {
				compute_pass.bind_pipeline(spatial_reuse_pipeline);
				compute_pass.bind_samplers(
					0,
					ssgi_target.temporal_reservoir_texture1.current().bind_with_sampler(nearest_sampler),
					ssgi_target.temporal_reservoir_texture2.current().bind_with_sampler(nearest_sampler),
					ssgi_target.temporal_reservoir_texture3.current().bind_with_sampler(nearest_sampler),
					ssgi_target.temporal_reservoir_texture4.current().bind_with_sampler(nearest_sampler),
					ssgi_target.spatial_reservoir_texture1.prev().bind_with_sampler(nearest_sampler),
					ssgi_target.spatial_reservoir_texture2.prev().bind_with_sampler(nearest_sampler),
					ssgi_target.spatial_reservoir_texture3.prev().bind_with_sampler(nearest_sampler),
					ssgi_target.spatial_reservoir_texture4.prev().bind_with_sampler(nearest_sampler),
					gbuffer.depth_value_texture.current().bind_with_sampler(nearest_sampler),
					gbuffer.depth_value_texture.prev().bind_with_sampler(linear_sampler),
					gbuffer.lighting_info_texture->bind_with_sampler(nearest_sampler),
					noise_texture.bind_with_sampler(noise_sampler),
					ssgi_target.temporal_reservoir_texture3.prev().bind_with_sampler(nearest_sampler)
				);
				compute_pass.dispatch(dispatch_size.x, dispatch_size.y, 1);
			}
		);
		command_buffer.pop_debug_group();

		return std::move(result).transform_error(util::Error::forward_fn("Spatial reuse pass failed"));
	}

	std::expected<void, util::Error> SSGI::run_radiance_composite(
		const gpu::Command_buffer& command_buffer,
		const target::Gbuffer& gbuffer,
		const target::SSGI& ssgi_target,
		const Param& param,
		glm::u32vec2 resolution
	) const noexcept
	{
		const Radiance_composite_param radiance_composite_param =
			Radiance_composite_param::from_param(param, resolution);
		command_buffer.push_uniform_to_compute(0, util::as_bytes(radiance_composite_param));

		const auto dispatch_size = (radiance_composite_param.comp_resolution + 15u) / 16u;

		const SDL_GPUStorageTextureReadWriteBinding write_binding_texture{
			.texture = ssgi_target.radiance_texture.current(),
			.mip_level = 0,
			.layer = 0,
			.cycle = true,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		command_buffer.push_debug_group("Radiance Composite Pass");
		auto result = command_buffer.run_compute_pass(
			std::array{write_binding_texture},
			{},
			[this, &gbuffer, &ssgi_target, dispatch_size](const gpu::Compute_pass& compute_pass) {
				compute_pass.bind_pipeline(radiance_composite_pipeline);
				compute_pass.bind_samplers(
					0,
					gbuffer.lighting_info_texture->bind_with_sampler(nearest_sampler),
					ssgi_target.radiance_texture.prev().bind_with_sampler(nearest_sampler),
					gbuffer.depth_value_texture.prev().bind_with_sampler(nearest_sampler),
					gbuffer.depth_value_texture.current().bind_with_sampler(nearest_sampler),
					gbuffer.albedo_texture->bind_with_sampler(nearest_sampler),
					ssgi_target.spatial_reservoir_texture1.current().bind_with_sampler(nearest_sampler),
					ssgi_target.spatial_reservoir_texture2.current().bind_with_sampler(nearest_sampler),
					ssgi_target.spatial_reservoir_texture3.prev().bind_with_sampler(nearest_sampler),
					ssgi_target.spatial_reservoir_texture4.current().bind_with_sampler(nearest_sampler)
				);
				compute_pass.dispatch(dispatch_size.x, dispatch_size.y, 1);
			}
		);
		command_buffer.pop_debug_group();

		return std::move(result).transform_error(util::Error::forward_fn("Radiance composite pass failed"));
	}

	std::expected<void, util::Error> SSGI::run_radiance_upsample(
		const gpu::Command_buffer& command_buffer,
		const target::Gbuffer& gbuffer,
		const target::SSGI& ssgi_target,
		const Param& param,
		glm::u32vec2 resolution
	) const noexcept
	{
		const Radiance_upsample_param radiance_upsample_param =
			Radiance_upsample_param::from_param(param, resolution);
		command_buffer.push_uniform_to_compute(0, util::as_bytes(radiance_upsample_param));

		const auto dispatch_size = (radiance_upsample_param.full_resolution + 15u) / 16u;

		const SDL_GPUStorageTextureReadWriteBinding write_binding_texture{
			.texture = *ssgi_target.fullres_radiance_texture,
			.mip_level = 0,
			.layer = 0,
			.cycle = true,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		command_buffer.push_debug_group("Radiance Upsample Pass");
		auto result = command_buffer.run_compute_pass(
			std::array{write_binding_texture},
			{},
			[this, &ssgi_target, &gbuffer, dispatch_size](const gpu::Compute_pass& compute_pass) {
				compute_pass.bind_pipeline(radiance_upsample_pipeline);
				compute_pass.bind_samplers(
					0,
					ssgi_target.radiance_texture.current().bind_with_sampler(nearest_sampler),
					gbuffer.depth_value_texture.current().bind_with_sampler(nearest_sampler),
					gbuffer.albedo_texture->bind_with_sampler(nearest_sampler),
					gbuffer.lighting_info_texture->bind_with_sampler(nearest_sampler)
				);
				compute_pass.dispatch(dispatch_size.x, dispatch_size.y, 1);
			}
		);
		command_buffer.pop_debug_group();

		return std::move(result).transform_error(util::Error::forward_fn("Radiance upsample pass failed"));
	}

	std::expected<void, util::Error> SSGI::render_radiance_add(
		const gpu::Command_buffer& command_buffer,
		const target::Light_buffer& light_buffer,
		const target::SSGI& ssgi_target,
		glm::u32vec2 resolution [[maybe_unused]]
	) const noexcept
	{
		command_buffer.push_debug_group("Radiance Add Pass");
		auto result = radiance_add_pass.render(
			command_buffer,
			light_buffer.light_texture.current(),
			std::array{ssgi_target.fullres_radiance_texture->bind_with_sampler(linear_sampler)},
			{},
			{}
		);
		command_buffer.pop_debug_group();
		return std::move(result).transform_error(util::Error::forward_fn("Radiance add pass failed"));
	}
}