#include "graphic/aa/fxaa.hpp"
#include "asset/shader/fxaa.frag.hpp"

namespace graphic::aa
{
	static constexpr gpu::Sampler::Create_info sampler_info = {
		.mipmap_mode = gpu::Sampler::Mipmap_mode::Nearest,
		.address_mode_u = gpu::Sampler::Address_mode::Clamp_to_edge,
		.address_mode_v = gpu::Sampler::Address_mode::Clamp_to_edge,
		.address_mode_w = gpu::Sampler::Address_mode::Clamp_to_edge
	};

	FXAA::FXAA(Fullscreen_pass fxaa_pass, gpu::Sampler sampler) noexcept :
		sampler(std::move(sampler)),
		fxaa_pass(std::move(fxaa_pass))
	{}

	std::expected<FXAA, util::Error> FXAA::create(SDL_GPUDevice* device, SDL_GPUTextureFormat format) noexcept
	{
		const auto shader_to_pass = [device, format](gpu::Graphic_shader fragment_shader) {
			return graphic::Fullscreen_pass::create(
				device,
				fragment_shader,
				gpu::Texture::Format{
					.type = SDL_GPU_TEXTURETYPE_2D,
					.format = format,
					.usage = {.color_target = true}
				},
				{.clear_before_render = false}
			);
		};

		auto fullscreen_pass =
			gpu::Graphic_shader::create(
				device,
				std::as_bytes(shader_asset::fxaa_frag),
				gpu::Graphic_shader::Stage::Fragment,
				1,
				0,
				0,
				0
			)
				.and_then(shader_to_pass);
		if (!fullscreen_pass) return fullscreen_pass.error().propagate("Create FXAA fullscreen pass failed");

		auto sampler = gpu::Sampler::create(device, sampler_info);
		if (!sampler) return sampler.error().propagate("Create FXAA sampler failed");

		return FXAA(std::move(*fullscreen_pass), std::move(*sampler));
	}

	std::expected<void, util::Error> FXAA::run_antialiasing(
		SDL_GPUDevice* device [[maybe_unused]],
		const gpu::Command_buffer& command_buffer,
		SDL_GPUTexture* source,
		SDL_GPUTexture* target,
		glm::u32vec2 size [[maybe_unused]]
	) noexcept
	{
		const auto texture_binding = std::to_array<SDL_GPUTextureSamplerBinding>({
			{.texture = source, .sampler = sampler}
		});

		return fxaa_pass.render(command_buffer, target, texture_binding, std::nullopt, std::nullopt);
	}
}