#include "pipeline/directional-light.hpp"
#include "asset/shader/directional-light.frag.hpp"
#include "target/light.hpp"
#include "util/as-byte.hpp"
#include <SDL3/SDL_gpu.h>

namespace pipeline
{
	std::expected<Directional_light, util::Error> Directional_light::create(SDL_GPUDevice* device) noexcept
	{
		const gpu::Sampler::Create_info sampler_create_info{
			.min_filter = gpu::Sampler::Filter::Nearest,
			.mag_filter = gpu::Sampler::Filter::Nearest,
			.mipmap_mode = gpu::Sampler::Mipmap_mode::Nearest,
			.max_lod = 0.0f
		};

		const gpu::Sampler::Create_info shadow_sampler_create_info{
			.min_filter = gpu::Sampler::Filter::Linear,
			.mag_filter = gpu::Sampler::Filter::Linear,
			.mipmap_mode = gpu::Sampler::Mipmap_mode::Nearest,
			.address_mode_u = gpu::Sampler::Address_mode::Clamp_to_edge,
			.address_mode_v = gpu::Sampler::Address_mode::Clamp_to_edge,
			.address_mode_w = gpu::Sampler::Address_mode::Clamp_to_edge,
			.min_lod = 0.0f,
			.max_lod = 0.0f,
			.compare_op = gpu::Sampler::Compare_op::Greater_or_equal
		};

		auto sampler = gpu::Sampler::create(device, sampler_create_info);
		if (!sampler) return sampler.error().forward("Create tonemapping sampler failed");

		auto shadow_sampler = gpu::Sampler::create(device, shadow_sampler_create_info);
		if (!shadow_sampler) return shadow_sampler.error().forward("Create shadow sampler failed");

		auto fragment_shader = gpu::Graphics_shader::create(
			device,
			shader_asset::directional_light_frag,
			gpu::Graphics_shader::Stage::Fragment,
			6,
			0,
			0,
			1
		);
		if (!fragment_shader)
			return fragment_shader.error().forward("Create directional light fragment shader failed");

		auto fullscreen_pass = graphics::Fullscreen_pass<false>::create(
			device,
			*fragment_shader,
			target::Light_buffer::light_buffer_format,
			"Directional Light Pipeline",
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
		if (!fullscreen_pass)
			return fullscreen_pass.error().forward("Create directional light fullscreen pass failed");

		return Directional_light(
			std::move(*fullscreen_pass),
			std::move(*sampler),
			std::move(*shadow_sampler)
		);
	}

	void Directional_light::render(
		const gpu::Command_buffer& command_buffer,
		const gpu::Render_pass& render_pass,
		const target::Gbuffer& gbuffer,
		const target::Shadow& shadow,
		const Params& params
	) const noexcept
	{
		const auto sampler_bindings = std::array{
			gbuffer.albedo_texture->bind_with_sampler(sampler),
			gbuffer.lighting_info_texture->bind_with_sampler(sampler),
			gbuffer.depth_value_texture.current().bind_with_sampler(sampler),
			shadow.depth_texture_level0->bind_with_sampler(shadow_sampler),
			shadow.depth_texture_level1->bind_with_sampler(shadow_sampler),
			shadow.depth_texture_level2->bind_with_sampler(shadow_sampler)
		};

		command_buffer.push_uniform_to_fragment(0, util::as_bytes(params));

		command_buffer.push_debug_group("Directional Light Pass");
		fullscreen_pass
			.render_to_renderpass(render_pass, std::span(sampler_bindings), std::nullopt, std::nullopt);
		command_buffer.pop_debug_group();
	}
}