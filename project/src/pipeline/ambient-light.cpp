#include "pipeline/ambient-light.hpp"

#include "asset/shader/ambient-light.frag.hpp"
#include "gpu/sampler.hpp"
#include "graphics/util/fullscreen-pass.hpp"
#include "target/gbuffer.hpp"
#include "target/light.hpp"
#include "util/as-byte.hpp"

#include <SDL3/SDL_gpu.h>
#include <optional>

namespace pipeline
{
	std::expected<Ambient_light, util::Error> Ambient_light::create(SDL_GPUDevice* device) noexcept
	{
		const gpu::Sampler::Create_info sampler_nearest_create_info{
			.min_filter = gpu::Sampler::Filter::Nearest,
			.mag_filter = gpu::Sampler::Filter::Nearest,
			.mipmap_mode = gpu::Sampler::Mipmap_mode::Nearest,
			.max_lod = 0.0f
		};
		auto sampler_nearest = gpu::Sampler::create(device, sampler_nearest_create_info);
		if (!sampler_nearest) return sampler_nearest.error().forward("Create sampler failed");

		const gpu::Sampler::Create_info sampler_linear_create_info{
			.min_filter = gpu::Sampler::Filter::Linear,
			.mag_filter = gpu::Sampler::Filter::Linear,
			.mipmap_mode = gpu::Sampler::Mipmap_mode::Linear,
			.max_lod = 0.0f
		};
		auto sampler_linear = gpu::Sampler::create(device, sampler_linear_create_info);
		if (!sampler_linear) return sampler_linear.error().forward("Create sampler failed");

		auto shader = gpu::Graphics_shader::create(
			device,
			shader_asset::ambient_light_frag,
			gpu::Graphics_shader::Stage::Fragment,
			3,
			0,
			0,
			1
		);
		if (!shader) return shader.error().forward("Create environment light shader failed");

		auto fullscreen_pass = graphics::Fullscreen_pass<false>::create(
			device,
			*shader,
			target::Light_buffer::light_buffer_format,
			"Ambient Light Pipeline",
			graphics::Fullscreen_blend_mode::Add,
			graphics::Fullscreen_stencil_state{
				.depth_format = target::Gbuffer::depth_format.format,
				.enable_stencil_test = true,
				.compare_mask = 0xFF,
				.write_mask = 0x00,
				.compare_op = SDL_GPU_COMPAREOP_EQUAL,
				.reference = 0x01
			}
		);
		if (!fullscreen_pass) return fullscreen_pass.error().forward("Create fullscreen pass failed");

		return Ambient_light(
			std::move(*fullscreen_pass),
			std::move(*sampler_nearest),
			std::move(*sampler_linear)
		);
	}

	void Ambient_light::render(
		const gpu::Command_buffer& command_buffer,
		const gpu::Render_pass& render_pass,
		const target::Gbuffer& gbuffer,
		const target::AO& ao,
		const Param& param
	) const noexcept
	{
		command_buffer.push_uniform_to_fragment(0, util::as_bytes(param));

		const std::array bindings = {
			gbuffer.albedo_texture->bind_with_sampler(sampler_nearest),
			gbuffer.lighting_info_texture->bind_with_sampler(sampler_nearest),
			ao.halfres_ao_texture.current().bind_with_sampler(sampler_nearest)
		};

		command_buffer.push_debug_group("Ambient Light Pass");
		fullscreen_pass.render_to_renderpass(render_pass, bindings, std::nullopt, std::nullopt);
		command_buffer.pop_debug_group();
	}
}