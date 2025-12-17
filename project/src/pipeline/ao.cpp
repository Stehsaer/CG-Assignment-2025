#include "pipeline/ao.hpp"

#include "asset/shader/ao.frag.hpp"
#include "gpu/graphics-pipeline.hpp"
#include "util/as-byte.hpp"

#include <random>

namespace pipeline
{
	AO::Uniform_params AO::Uniform_params::from(const Params& params) noexcept
	{
		static thread_local std::mt19937 generator{std::random_device{}()};
		static thread_local std::uniform_real_distribution<float> distribution{0.0f, 1.0f};

		const float random_seed = distribution(generator);

		return {
			.camera_mat_inv = params.camera_mat_inv,
			.view_mat = params.view_mat,
			.proj_mat_inv = params.proj_mat_inv,
			.prev_camera_mat = params.prev_camera_mat,
			.random_seed = random_seed,
			.radius = params.radius,
			.blend_alpha = params.blend_alpha
		};
	}

	static std::expected<gpu::Graphics_shader, util::Error> create_ao_shader(SDL_GPUDevice* device) noexcept
	{
		return gpu::Graphics_shader::
			create(device, shader_asset::ao_frag, gpu::Graphics_shader::Stage::Fragment, 4, 0, 0, 1)
				.transform_error(util::Error::forward_fn("Create AO fragment shader failed"));
	}

	std::expected<AO, util::Error> AO::create(SDL_GPUDevice* device) noexcept
	{
		const auto shader = create_ao_shader(device);
		if (!shader) return shader.error();

		const gpu::Sampler::Create_info sampler_linear_create_info{
			.min_filter = gpu::Sampler::Filter::Linear,
			.mag_filter = gpu::Sampler::Filter::Linear,
			.mipmap_mode = gpu::Sampler::Mipmap_mode::Nearest,
			.max_lod = 0.0f
		};

		const gpu::Sampler::Create_info sampler_nearest_create_info{
			.min_filter = gpu::Sampler::Filter::Nearest,
			.mag_filter = gpu::Sampler::Filter::Nearest,
			.mipmap_mode = gpu::Sampler::Mipmap_mode::Nearest,
			.max_lod = 0.0f
		};

		auto sampler_linear = gpu::Sampler::create(device, sampler_linear_create_info);
		if (!sampler_linear) return sampler_linear.error().forward("Create tonemapping sampler failed");

		auto sampler_nearest = gpu::Sampler::create(device, sampler_nearest_create_info);
		if (!sampler_nearest) return sampler_nearest.error().forward("Create tonemapping sampler failed");

		auto fullscreen_pass =
			graphics::Fullscreen_pass<false>::create(device, *shader, target::AO::ao_format, {});
		if (!fullscreen_pass) return fullscreen_pass.error().forward("Create AO fullscreen pass failed");

		return AO(std::move(*sampler_linear), std::move(*sampler_nearest), std::move(*fullscreen_pass));
	}

	void AO::render(
		const gpu::Command_buffer& command_buffer,
		const gpu::Render_pass& render_pass,
		const target::AO& ao_target,
		const target::Gbuffer& gbuffer,
		const Params& params
	) const noexcept
	{
		const std::array texture_bindings = {
			gbuffer.depth_value_texture.current().bind_with_sampler(sampler_linear),
			gbuffer.lighting_info_texture->bind_with_sampler(sampler_nearest),
			gbuffer.depth_value_texture.prev().bind_with_sampler(sampler_linear),
			ao_target.halfres_ao_texture.prev().bind_with_sampler(sampler_linear)
		};

		const auto uniform_params = Uniform_params::from(params);
		command_buffer.push_uniform_to_fragment(0, util::as_bytes(uniform_params));

		command_buffer.push_debug_group("AO Pass");
		fullscreen_pass.render_to_renderpass(render_pass, texture_bindings, {}, {});
		command_buffer.pop_debug_group();
	}
}