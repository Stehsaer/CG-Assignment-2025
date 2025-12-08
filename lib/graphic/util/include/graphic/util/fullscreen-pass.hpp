#pragma once

#include <glm/glm.hpp>
#include <gpu.hpp>
#include <optional>
#include <util/error.hpp>

namespace graphic
{
	class Fullscreen_pass
	{
	  public:

		// Fullscreen pass configuration
		struct Config
		{
			bool clear_before_render = true;
			glm::vec4 clear_color = {0, 0, 0, 1};
			bool do_cycle = true;
		};

		Fullscreen_pass(const Fullscreen_pass&) = delete;
		Fullscreen_pass& operator=(const Fullscreen_pass&) = delete;
		Fullscreen_pass(Fullscreen_pass&&) = default;
		Fullscreen_pass& operator=(Fullscreen_pass&&) = default;

		///
		/// @brief Create a fullscreen pass
		///
		/// @param fragment Fragment shader
		/// @param target_format Target texture format
		/// @param config Configuration
		/// @return Fullscreen pass object, or error if failed
		///
		static std::expected<Fullscreen_pass, util::Error> create(
			SDL_GPUDevice* device,
			const gpu::Graphic_shader& fragment,
			gpu::Texture::Format target_format,
			Config config
		) noexcept;

		///
		/// @brief Render the fullscreen pass
		/// @note Prepare the resources and push the uniforms before render
		///
		/// @param target_texture Target texture
		/// @param samplers Sampler bindings, can be empty
		/// @param storage_textures Storage texture bindings, can be empty
		/// @param storage_buffers Storage buffer bindings, can be empty
		///
		std::expected<void, util::Error> render(
			const gpu::Command_buffer& command_buffer,
			SDL_GPUTexture* target_texture,
			std::optional<std::span<const SDL_GPUTextureSamplerBinding>> samplers,
			std::optional<std::span<SDL_GPUTexture* const>> storage_textures,
			std::optional<std::span<SDL_GPUBuffer* const>> storage_buffers
		) noexcept;

	  private:

		Fullscreen_pass(gpu::Graphics_pipeline pipeline, gpu::Buffer vertex_buffer, Config config) noexcept;

		gpu::Buffer fullscreen_vertex_buffer;
		gpu::Graphics_pipeline pipeline;
		Config config;
	};
}