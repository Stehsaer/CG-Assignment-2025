#pragma once

#include <SDL3/SDL_gpu.h>
#include <expected>
#include <optional>
#include <span>

#include "resource-box.hpp"
#include "util/error.hpp"

namespace gpu
{
	///
	/// @brief graphics Shader Object
	///
	///
	class GraphicsShader : public ResourceBox<SDL_GPUShader>
	{
	  public:

		GraphicsShader(const GraphicsShader&) = delete;
		GraphicsShader& operator=(const GraphicsShader&) = delete;
		GraphicsShader(GraphicsShader&&) noexcept = default;
		GraphicsShader& operator=(GraphicsShader&&) noexcept = default;
		~GraphicsShader() noexcept = default;

		enum class Stage
		{
			Vertex = SDL_GPU_SHADERSTAGE_VERTEX,
			Fragment = SDL_GPU_SHADERSTAGE_FRAGMENT
		};

		///
		/// @brief Creates a graphics shader
		///
		static std::expected<GraphicsShader, util::Error> create(
			SDL_GPUDevice* device,
			std::span<const std::byte> shader_data,
			Stage stage,
			uint32_t num_samplers,
			uint32_t num_storage_textures,
			uint32_t num_storage_buffers,
			uint32_t num_uniform_buffers
		) noexcept;

	  private:

		using ResourceBox<SDL_GPUShader>::ResourceBox;
	};

	///
	/// @brief GPU graphics Pipeline
	///
	///
	class GraphicsPipeline : public ResourceBox<SDL_GPUGraphicsPipeline>
	{
	  public:

		GraphicsPipeline(const GraphicsPipeline&) = delete;
		GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
		GraphicsPipeline(GraphicsPipeline&&) noexcept = default;
		GraphicsPipeline& operator=(GraphicsPipeline&&) noexcept = default;
		~GraphicsPipeline() noexcept = default;

		///
		/// @brief Depth stencil state
		///
		///
		struct DepthStencilState
		{
			static constexpr SDL_GPUStencilOpState default_stencil_op_state = {
				.fail_op = SDL_GPU_STENCILOP_KEEP,
				.pass_op = SDL_GPU_STENCILOP_KEEP,
				.depth_fail_op = SDL_GPU_STENCILOP_KEEP,
				.compare_op = SDL_GPU_COMPAREOP_ALWAYS
			};

			SDL_GPUTextureFormat format;
			SDL_GPUCompareOp compare_op;
			SDL_GPUStencilOpState back_stencil_state = default_stencil_op_state;
			SDL_GPUStencilOpState front_stencil_state = default_stencil_op_state;
			Uint8 compare_mask;
			Uint8 write_mask;
			bool enable_depth_test;
			bool enable_depth_write;
			bool enable_stencil_test;

			SDL_GPUDepthStencilState to_sdl(this const DepthStencilState& self) noexcept;
		};

		///
		/// @brief Creates a graphics pipeline
		///
		/// @param device GPU device
		/// @param vertex_shader Vertex shader
		/// @param fragment_shader Fragment shader
		/// @param primitive_type Primitive type used for drawing
		/// @param multisample_count Multisample count used
		/// @param rasterizer_state Rasterizer state
		/// @param vertex_attributes Vertex attribute descriptions in the vertex shader
		/// @param vertex_buffer_descs Vertex buffer descriptions
		/// @param color_target_descs Render target descriptions
		/// @param depth_stencil_state (Optional) Depth stencil state
		/// @return Pipeline object on success, or error on failure
		///
		static std::expected<GraphicsPipeline, util::Error> create(
			SDL_GPUDevice* device,
			const GraphicsShader& vertex_shader,
			const GraphicsShader& fragment_shader,
			SDL_GPUPrimitiveType primitive_type,
			SDL_GPUSampleCount multisample_count,
			const SDL_GPURasterizerState& rasterizer_state,
			std::span<const SDL_GPUVertexAttribute> vertex_attributes,
			std::span<const SDL_GPUVertexBufferDescription> vertex_buffer_descs,
			std::span<const SDL_GPUColorTargetDescription> color_target_descs,
			const std::optional<DepthStencilState>& depth_stencil_state,
			const std::string& name
		) noexcept;

	  private:

		using ResourceBox<SDL_GPUGraphicsPipeline>::ResourceBox;
	};
}