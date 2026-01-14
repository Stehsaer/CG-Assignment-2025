#include "graphics/aa/fxaa.hpp"
#include "asset/shader/fxaa.frag.hpp"

#include <array>

namespace graphics::aa
{
	static constexpr gpu::Sampler::CreateInfo sampler_info = {
		.mipmap_mode = gpu::Sampler::MipmapMode::Nearest,
		.address_mode_u = gpu::Sampler::AddressMode::Clamp_to_edge,
		.address_mode_v = gpu::Sampler::AddressMode::Clamp_to_edge,
		.address_mode_w = gpu::Sampler::AddressMode::Clamp_to_edge
	};

	FXAA::FXAA(FullscreenPass<true> fxaa_pass, gpu::Sampler sampler) noexcept :
		sampler(std::move(sampler)),
		fxaa_pass(std::move(fxaa_pass))
	{}

	std::expected<FXAA, util::Error> FXAA::create(SDL_GPUDevice* device, SDL_GPUTextureFormat format) noexcept
	{
		const auto shader_to_pass = [device, format](gpu::GraphicsShader fragment_shader) {
			return graphics::FullscreenPass<true>::create(
				device,
				fragment_shader,
				gpu::Texture::Format{
					.type = SDL_GPU_TEXTURETYPE_2D,
					.format = format,
					.usage = {.color_target = true}
				},
				{.clear_before_render = false},
				"FXAA Pipeline"
			);
		};

		auto FullscreenPass =
			gpu::GraphicsShader::create(
				device,
				std::as_bytes(shader_asset::fxaa_frag),
				gpu::GraphicsShader::Stage::Fragment,
				1,
				0,
				0,
				0
			)
				.and_then(shader_to_pass);
		if (!FullscreenPass) return FullscreenPass.error().forward("Create FXAA fullscreen pass failed");

		auto sampler = gpu::Sampler::create(device, sampler_info);
		if (!sampler) return sampler.error().forward("Create FXAA sampler failed");

		return FXAA(std::move(*FullscreenPass), std::move(*sampler));
	}

	std::expected<void, util::Error> FXAA::run_antialiasing(
		SDL_GPUDevice* device [[maybe_unused]],
		const gpu::CommandBuffer& command_buffer,
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