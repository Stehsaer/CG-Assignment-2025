#pragma once

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <optional>

#include "gpu/command-buffer.hpp"
#include "gpu/graphics-pipeline.hpp"
#include "util/error.hpp"

namespace graphics
{
	enum class Fullscreen_blend_mode
	{
		Overwrite,  // Completely overwrite
		Add         // Additive blend
	};

	struct Fullscreen_stencil_state
	{
		SDL_GPUTextureFormat depth_format;
		bool enable_stencil_test = false;
		uint8_t compare_mask = 0xFF;
		uint8_t write_mask = 0xFF;
		SDL_GPUCompareOp compare_op = SDL_GPU_COMPAREOP_ALWAYS;
		uint8_t reference = 0x00;

		gpu::Graphics_pipeline::Depth_stencil_state to_depth_stencil_state() const noexcept;
	};

	///
	/// @brief Fullscreen Pass
	///
	/// @tparam Has_render_pass `true` if the pass uses its own render pass, `false` otherwise
	///
	template <bool Has_render_pass>
	class Fullscreen_pass;

	template <>
	class Fullscreen_pass<false>
	{
	  public:

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
		static std::expected<Fullscreen_pass<false>, util::Error> create(
			SDL_GPUDevice* device,
			const gpu::Graphics_shader& fragment,
			gpu::Texture::Format target_format,
			const std::string& name,
			Fullscreen_blend_mode blend_mode = Fullscreen_blend_mode::Overwrite,
			std::optional<Fullscreen_stencil_state> stencil_state = std::nullopt
		) noexcept;

		///
		/// @brief Render directly to a render pass
		/// @note In this usage, render-pass-related configurations in `Config` will be ignored
		///
		/// @param render_pass Render pass
		/// @param samplers Sampler bindings, can be empty
		/// @param storage_textures Storage texture bindings, can be empty
		/// @param storage_buffers Storage buffer bindings, can be empty
		///
		void render_to_renderpass(
			const gpu::Render_pass& render_pass,
			std::optional<std::span<const SDL_GPUTextureSamplerBinding>> samplers,
			std::optional<std::span<SDL_GPUTexture* const>> storage_textures,
			std::optional<std::span<SDL_GPUBuffer* const>> storage_buffers
		) const noexcept;

	  private:

		Fullscreen_pass(
			gpu::Graphics_pipeline pipeline,
			gpu::Buffer vertex_buffer,
			std::optional<uint8_t> stencil_ref
		) noexcept :
			fullscreen_vertex_buffer(std::move(vertex_buffer)),
			pipeline(std::move(pipeline)),
			stencil_ref(stencil_ref)
		{}

		gpu::Buffer fullscreen_vertex_buffer;
		gpu::Graphics_pipeline pipeline;
		std::optional<uint8_t> stencil_ref;
	};

	template <>
	class Fullscreen_pass<true>
	{
	  public:

		struct Config
		{
			bool clear_before_render = true;
			glm::vec4 clear_color = {0, 0, 0, 1};
			bool do_cycle = true;
			Fullscreen_blend_mode blend_mode = Fullscreen_blend_mode::Overwrite;
			std::optional<Fullscreen_stencil_state> stencil_state = std::nullopt;
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
		static std::expected<Fullscreen_pass<true>, util::Error> create(
			SDL_GPUDevice* device,
			const gpu::Graphics_shader& fragment,
			gpu::Texture::Format target_format,
			Config config,
			const std::string& name
		) noexcept;

		///
		/// @brief Render the fullscreen pass
		/// @note Prepare the resources and push the uniforms before render
		///
		/// @param command_buffer Command buffer
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
		) const noexcept;

	  private:

		Fullscreen_pass(Fullscreen_pass<false> base_pass, Config config) noexcept :
			base_pass(std::move(base_pass)),
			config(config)
		{}

		Fullscreen_pass<false> base_pass;
		Config config;

		SDL_GPULoadOp select_loadop() const noexcept;
	};
}