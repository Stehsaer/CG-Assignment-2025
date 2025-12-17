#pragma once

#include "gpu/sampler.hpp"
#include "graphics/util/fullscreen-pass.hpp"

#include <glm/glm.hpp>

namespace pipeline
{
	class Debug_pipeline
	{
	  public:

		static std::expected<Debug_pipeline, util::Error> create(
			SDL_GPUDevice* device,
			gpu::Texture::Format format
		) noexcept;

		void render_channels(
			const gpu::Command_buffer& command_buffer,
			const gpu::Render_pass& render_pass,
			SDL_GPUTexture* input_texture,
			glm::u32vec2 size,
			uint8_t channel_count
		) const noexcept;

	  private:

		graphics::Fullscreen_pass<false> fullscreen_pass;
		gpu::Sampler sampler;

		Debug_pipeline(graphics::Fullscreen_pass<false> fullscreen_pass, gpu::Sampler sampler) noexcept :
			fullscreen_pass(std::move(fullscreen_pass)),
			sampler(std::move(sampler))
		{}

	  public:

		Debug_pipeline(const Debug_pipeline&) = delete;
		Debug_pipeline(Debug_pipeline&&) = default;
		Debug_pipeline& operator=(const Debug_pipeline&) = delete;
		Debug_pipeline& operator=(Debug_pipeline&&) = default;
	};
}