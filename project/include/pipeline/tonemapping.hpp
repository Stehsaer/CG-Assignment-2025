#pragma once

#include "gpu/command-buffer.hpp"
#include "gpu/sampler.hpp"
#include "graphics/util/fullscreen-pass.hpp"
#include "target/auto-exposure.hpp"
#include "target/bloom.hpp"
#include "target/light.hpp"

#include <glm/glm.hpp>

namespace pipeline
{
	class Tonemapping
	{
	  public:

		struct Param
		{
			float exposure = 1.0f;
			float bloom_strength = 1.0f;
		};

		static std::expected<Tonemapping, util::Error> create(
			SDL_GPUDevice* device,
			SDL_GPUTextureFormat target_format
		) noexcept;

		std::expected<void, util::Error> render(
			const gpu::Command_buffer& command_buffer,
			const target::Light_buffer& light_buffer,
			const target::Auto_exposure& auto_exposure,
			const target::Bloom& bloom,
			SDL_GPUTexture* target_texture,
			const Param& param
		) const noexcept;

	  private:

		graphics::Fullscreen_pass<true> fullscreen_pass;
		gpu::Sampler nearest_sampler;
		gpu::Sampler linear_sampler;

		Tonemapping(
			graphics::Fullscreen_pass<true> fullscreen_pass,
			gpu::Sampler nearest_sampler,
			gpu::Sampler linear_sampler
		) :
			fullscreen_pass(std::move(fullscreen_pass)),
			nearest_sampler(std::move(nearest_sampler)),
			linear_sampler(std::move(linear_sampler))
		{}

	  public:

		Tonemapping(const Tonemapping&) = delete;
		Tonemapping(Tonemapping&&) = default;
		Tonemapping& operator=(const Tonemapping&) = delete;
		Tonemapping& operator=(Tonemapping&&) = default;
	};
}