#include "pipeline/debug.hpp"
#include "asset/shader/debug-single.frag.hpp"
#include "util/as-byte.hpp"

#include <SDL3/SDL_gpu.h>

namespace pipeline
{
	std::expected<Debug_pipeline, util::Error> Debug_pipeline::create(
		SDL_GPUDevice* device,
		gpu::Texture::Format format
	) noexcept
	{
		auto shader = gpu::Graphics_shader::create(
			device,
			shader_asset::debug_single_frag,
			gpu::Graphics_shader::Stage::Fragment,
			1,
			0,
			0,
			1
		);
		if (!shader) return shader.error().forward("Create debug shader failed");

		auto fullscreen_pass =
			graphics::Fullscreen_pass<false>::create(device, *shader, format, "Debug Pipeline");
		if (!fullscreen_pass) return fullscreen_pass.error().forward("Create fullscreen pass failed");

		const gpu::Sampler::Create_info sampler_create_info{
			.min_filter = gpu::Sampler::Filter::Nearest,
			.mag_filter = gpu::Sampler::Filter::Nearest,
			.mipmap_mode = gpu::Sampler::Mipmap_mode::Nearest,
			.max_lod = 0.0f
		};
		auto sampler = gpu::Sampler::create(device, sampler_create_info);
		if (!sampler) return sampler.error().forward("Create sampler failed");

		return Debug_pipeline(std::move(*fullscreen_pass), std::move(*sampler));
	}

	void Debug_pipeline::render_channels(
		const gpu::Command_buffer& command_buffer [[maybe_unused]],
		const gpu::Render_pass& render_pass,
		SDL_GPUTexture* input_texture,
		glm::u32vec2 size [[maybe_unused]],
		uint8_t channel_count
	) const noexcept
	{
		if (channel_count == 0 || channel_count > 4) return;

		const auto channel_count_u32 = static_cast<uint32_t>(channel_count);
		command_buffer.push_uniform_to_fragment(0, util::as_bytes(channel_count_u32));

		const auto bind = SDL_GPUTextureSamplerBinding{.texture = input_texture, .sampler = sampler};
		const std::array bindings = {bind};
		fullscreen_pass.render_to_renderpass(render_pass, bindings, {}, {});
	}
}