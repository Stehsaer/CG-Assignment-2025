#pragma once

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <optional>

#include "gpu/command-buffer.hpp"
#include "gpu/graphics-pipeline.hpp"
#include "util/error.hpp"

namespace graphics
{
	enum class FullscreenBlendMode
	{
		Overwrite,  // Completely overwrite
		Add         // Additive blend
	};

	struct FullscreenStencilState
	{
		SDL_GPUTextureFormat depth_format;
		bool enable_stencil_test = false;
		uint8_t compare_mask = 0xFF;
		uint8_t write_mask = 0xFF;
		SDL_GPUCompareOp compare_op = SDL_GPU_COMPAREOP_ALWAYS;
		uint8_t reference = 0x00;

		gpu::GraphicsPipeline::DepthStencilState to_depth_stencil_state() const noexcept;
	};

	///
	/// @brief Fullscreen Pass
	///
	/// @tparam HasRenderPass `true` if the pass uses its own render pass, `false` otherwise
	///
	template <bool HasRenderPass>
	class FullscreenPass;

	template <>
	class FullscreenPass<false>
	{
	  public:

		FullscreenPass(const FullscreenPass&) = delete;
		FullscreenPass& operator=(const FullscreenPass&) = delete;
		FullscreenPass(FullscreenPass&&) = default;
		FullscreenPass& operator=(FullscreenPass&&) = default;

		///
		/// @brief Create a fullscreen pass
		///
		/// @param fragment Fragment shader
		/// @param target_format Target texture format
		/// @param config Configuration
		/// @return Fullscreen pass object, or error if failed
		///
		static std::expected<FullscreenPass<false>, util::Error> create(
			SDL_GPUDevice* device,
			const gpu::GraphicsShader& fragment,
			gpu::Texture::Format target_format,
			const std::string& name,
			FullscreenBlendMode blend_mode = FullscreenBlendMode::Overwrite,
			std::optional<FullscreenStencilState> stencil_state = std::nullopt
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
			const gpu::RenderPass& render_pass,
			std::optional<std::span<const SDL_GPUTextureSamplerBinding>> samplers,
			std::optional<std::span<SDL_GPUTexture* const>> storage_textures,
			std::optional<std::span<SDL_GPUBuffer* const>> storage_buffers
		) const noexcept;

	  private:

		FullscreenPass(
			gpu::GraphicsPipeline pipeline,
			gpu::Buffer vertex_buffer,
			std::optional<uint8_t> stencil_ref
		) noexcept :
			fullscreen_vertex_buffer(std::move(vertex_buffer)),
			pipeline(std::move(pipeline)),
			stencil_ref(stencil_ref)
		{}

		gpu::Buffer fullscreen_vertex_buffer;
		gpu::GraphicsPipeline pipeline;
		std::optional<uint8_t> stencil_ref;
	};

	template <>
	class FullscreenPass<true>
	{
	  public:

		struct Config
		{
			bool clear_before_render = true;
			glm::vec4 clear_color = {0, 0, 0, 1};
			bool do_cycle = true;
			FullscreenBlendMode blend_mode = FullscreenBlendMode::Overwrite;
			std::optional<FullscreenStencilState> stencil_state = std::nullopt;
		};

		FullscreenPass(const FullscreenPass&) = delete;
		FullscreenPass& operator=(const FullscreenPass&) = delete;
		FullscreenPass(FullscreenPass&&) = default;
		FullscreenPass& operator=(FullscreenPass&&) = default;

		///
		/// @brief Create a fullscreen pass
		///
		/// @param fragment Fragment shader
		/// @param target_format Target texture format
		/// @param config Configuration
		/// @return Fullscreen pass object, or error if failed
		///
		static std::expected<FullscreenPass<true>, util::Error> create(
			SDL_GPUDevice* device,
			const gpu::GraphicsShader& fragment,
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
			const gpu::CommandBuffer& command_buffer,
			SDL_GPUTexture* target_texture,
			std::optional<std::span<const SDL_GPUTextureSamplerBinding>> samplers,
			std::optional<std::span<SDL_GPUTexture* const>> storage_textures,
			std::optional<std::span<SDL_GPUBuffer* const>> storage_buffers
		) const noexcept;

	  private:

		FullscreenPass(FullscreenPass<false> base_pass, Config config) noexcept :
			base_pass(std::move(base_pass)),
			config(config)
		{}

		FullscreenPass<false> base_pass;
		Config config;

		SDL_GPULoadOp select_loadop() const noexcept;
	};
}