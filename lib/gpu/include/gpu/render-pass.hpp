#pragma once

#include "buffer.hpp"
#include "graphics-pipeline.hpp"
#include "scoped-pass.hpp"

#include <SDL3/SDL_gpu.h>
#include <concepts>
#include <span>

namespace gpu
{
	class RenderPass : public ScopedPass<SDL_GPURenderPass>
	{
	  public:

		///
		/// @brief Binds a graphics pipeline
		/// @param pipeline Graphics pipeline
		///
		void bind_pipeline(const GraphicsPipeline& pipeline) const noexcept;

		///
		/// @brief Binds some vertex buffers
		///
		/// @param first_slot First binding slot
		/// @param bindings Binding information
		///
		void bind_vertex_buffers(
			uint32_t first_slot,
			std::span<const SDL_GPUBufferBinding> bindings
		) const noexcept;

		///
		/// @brief Bind some vertex buffers
		///
		/// @param first_slot First binding slot
		/// @param bindings Binding packs, the first one is bound to first_slot, and so on
		///
		template <std::convertible_to<SDL_GPUBufferBinding>... Args>
			requires(sizeof...(Args) > 0)
		void bind_vertex_buffers(uint32_t first_slot, Args&&... bindings) const noexcept
		{
			const std::array<SDL_GPUBufferBinding, sizeof...(bindings)> binding_arr = {
				std::forward<Args>(bindings)...
			};
			bind_vertex_buffers(first_slot, binding_arr);
		}

		///
		/// @brief Binds an index buffer
		///
		/// @param binding Buffer information
		/// @param element_size Size of index elements in the buffer
		///
		void bind_index_buffer(
			const SDL_GPUBufferBinding& binding,
			SDL_GPUIndexElementSize element_size
		) const noexcept;

		///
		/// @brief Binds samplers for the vertex shader
		///
		/// @param first_slot First binding slot
		/// @param bindings Binding information
		///
		void bind_vertex_samplers(
			uint32_t first_slot,
			std::span<const SDL_GPUTextureSamplerBinding> bindings
		) const noexcept;

		///
		/// @brief Bind some vertex samplers
		///
		/// @param first_slot First binding slot
		/// @param bindings Binding packs, the first one is bound to first_slot, and so on
		///
		template <std::convertible_to<SDL_GPUTextureSamplerBinding>... Args>
			requires(sizeof...(Args) > 0)
		void bind_vertex_samplers(uint32_t first_slot, Args&&... bindings) const noexcept
		{
			const std::array<SDL_GPUTextureSamplerBinding, sizeof...(bindings)> binding_arr = {
				std::forward<Args>(bindings)...
			};
			bind_vertex_samplers(first_slot, binding_arr);
		}

		///
		/// @brief Binds storage textures for the vertex shader
		///
		/// @param first_slot First binding slot
		/// @param textures Textures
		///
		void bind_vertex_storage_textures(
			uint32_t first_slot,
			std::span<SDL_GPUTexture* const> textures
		) const noexcept;

		///
		/// @brief Bind some vertex storage textures
		///
		/// @param first_slot First binding slot
		/// @param textures Texture packs, the first one is bound to first_slot, and so on
		///
		template <std::convertible_to<SDL_GPUTexture*>... Args>
			requires(sizeof...(Args) > 0)
		void bind_vertex_storage_textures(uint32_t first_slot, Args&&... textures) const noexcept
		{
			const std::array<SDL_GPUTexture*, sizeof...(Args)> texture_arr = {
				std::forward<Args>(textures)...
			};
			bind_vertex_storage_textures(first_slot, texture_arr);
		}

		///
		/// @brief Binds storage buffers for the vertex shader
		///
		/// @param first_slot First binding slot
		/// @param buffers Buffers
		///
		void bind_vertex_storage_buffers(
			uint32_t first_slot,
			std::span<SDL_GPUBuffer* const> buffers
		) const noexcept;

		///
		/// @brief Bind some vertex storage buffers
		///
		/// @param first_slot First binding slot
		/// @param buffers Buffer packs, the first one is bound to first_slot, and so on
		///
		template <std::convertible_to<SDL_GPUBuffer*>... Args>
			requires(sizeof...(Args) > 0)
		void bind_vertex_storage_buffers(uint32_t first_slot, Args&&... buffers) const noexcept
		{
			const std::array<SDL_GPUBuffer*, sizeof...(Args)> buffer_arr = {std::forward<Args>(buffers)...};
			bind_vertex_storage_buffers(first_slot, buffer_arr);
		}

		///
		/// @brief Binds samplers for the fragment shader
		///
		/// @param first_slot First binding slot
		/// @param bindings Sampler binding information
		///
		void bind_fragment_samplers(
			uint32_t first_slot,
			std::span<const SDL_GPUTextureSamplerBinding> bindings
		) const noexcept;

		///
		/// @brief Bind some fragment samplers
		///
		/// @param first_slot First binding slot
		/// @param bindings Binding packs, the first one is bound to first_slot, and so on
		///
		template <std::convertible_to<SDL_GPUTextureSamplerBinding>... Args>
			requires(sizeof...(Args) > 0)
		void bind_fragment_samplers(uint32_t first_slot, Args&&... bindings) const noexcept
		{
			const std::array<SDL_GPUTextureSamplerBinding, sizeof...(bindings)> binding_arr = {
				std::forward<Args>(bindings)...
			};
			bind_fragment_samplers(first_slot, binding_arr);
		}

		///
		/// @brief Binds storage textures for the fragment shader
		///
		/// @param first_slot First binding slot
		/// @param textures Textures
		///
		void bind_fragment_storage_textures(
			uint32_t first_slot,
			std::span<SDL_GPUTexture* const> textures
		) const noexcept;

		///
		/// @brief Bind some fragment storage textures
		///
		/// @param first_slot First binding slot
		/// @param textures Texture packs, the first one is bound to first_slot, and so on
		///
		template <std::convertible_to<SDL_GPUTexture*>... Args>
			requires(sizeof...(Args) > 0)
		void bind_fragment_storage_textures(uint32_t first_slot, Args&&... textures) const noexcept
		{
			const std::array<SDL_GPUTexture*, sizeof...(Args)> texture_arr = {
				std::forward<Args>(textures)...
			};
			bind_fragment_storage_textures(first_slot, texture_arr);
		}

		///
		/// @brief Binds storage buffers for the fragment shader
		///
		/// @param first_slot First binding slot
		/// @param buffers Buffers
		///
		void bind_fragment_storage_buffers(
			uint32_t first_slot,
			std::span<SDL_GPUBuffer* const> buffers
		) const noexcept;

		///
		/// @brief Bind some fragment storage buffers
		///
		/// @param first_slot First binding slot
		/// @param buffers Buffer packs, the first one is bound to first_slot, and so on
		///
		template <std::convertible_to<SDL_GPUBuffer*>... Args>
			requires(sizeof...(Args) > 0)
		void bind_fragment_storage_buffers(uint32_t first_slot, Args&&... buffers) const noexcept
		{
			const std::array<SDL_GPUBuffer*, sizeof...(Args)> buffer_arr = {std::forward<Args>(buffers)...};
			bind_fragment_storage_buffers(first_slot, buffer_arr);
		}

		///
		/// @brief Draws primitives using indices
		///
		/// @param index_count Number of indices
		/// @param index_offset Index offset
		/// @param instance_count Number of instances
		/// @param instance_offset Instance offset
		/// @param vertex_offset Offset added to indices
		///
		void draw_indexed(
			uint32_t index_count,
			uint32_t index_offset,
			uint32_t instance_count,
			uint32_t instance_offset,
			int32_t vertex_offset
		) const noexcept;

		///
		/// @brief Draws primitives
		///
		/// @param vertex_count Number of vertices
		/// @param vertex_offset Vertex offset
		/// @param instance_count Number of instances
		/// @param instance_offset Instance offset
		///
		void draw(
			uint32_t vertex_count,
			uint32_t vertex_offset,
			uint32_t instance_count,
			uint32_t instance_offset
		) const noexcept;

		///
		/// @brief Draws primitives indirectly
		///
		/// @param buffer Buffer storing draw parameters
		/// @param count Number of draw calls
		/// @param offset Buffer offset
		///
		void draw_indirect(const Buffer& buffer, uint32_t count, uint32_t offset) const noexcept;

		///
		/// @brief Draws primitives indirectly and indexed
		///
		/// @param buffer Buffer storing draw parameters
		/// @param count Number of draw calls
		/// @param offset Buffer offset
		///
		void draw_indexed_indirect(const Buffer& buffer, uint32_t count, uint32_t offset) const noexcept;

		///
		/// @brief Sets the viewport
		///
		/// @param viewport Viewport
		///
		void set_viewport(const SDL_GPUViewport& viewport) const noexcept;

		///
		/// @brief Sets the scissor rectangle
		///
		/// @param scissor Scissor rectangle
		///
		void set_scissor(const SDL_Rect& scissor) const noexcept;

		///
		/// @brief Sets blend constants
		///
		/// @param blend_constants Blend constants
		///
		void set_blend_constants(const SDL_FColor& blend_constants) const noexcept;

		///
		/// @brief Sets the stencil reference value
		///
		/// @param reference Reference value
		///
		void set_stencil_reference(uint8_t reference) const noexcept;

	  private:

		using ScopedPass<SDL_GPURenderPass>::ScopedPass;
	};
}