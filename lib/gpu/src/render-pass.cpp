#include "gpu/render-pass.hpp"

namespace gpu
{
	void RenderPass::bind_pipeline(const GraphicsPipeline& pipeline) const noexcept
	{
		assert(resource != nullptr);
		SDL_BindGPUGraphicsPipeline(resource, pipeline);
	}

	void RenderPass::bind_vertex_buffers(
		uint32_t first_slot,
		std::span<const SDL_GPUBufferBinding> bindings
	) const noexcept
	{
		assert(resource != nullptr);
		SDL_BindGPUVertexBuffers(
			resource,
			first_slot,
			bindings.data(),
			static_cast<uint32_t>(bindings.size())
		);
	}

	void RenderPass::bind_index_buffer(
		const SDL_GPUBufferBinding& binding,
		SDL_GPUIndexElementSize element_size
	) const noexcept
	{
		assert(resource != nullptr);
		SDL_BindGPUIndexBuffer(resource, &binding, element_size);
	}

	void RenderPass::bind_vertex_samplers(
		uint32_t first_slot,
		std::span<const SDL_GPUTextureSamplerBinding> bindings
	) const noexcept
	{
		assert(resource != nullptr);
		SDL_BindGPUVertexSamplers(
			resource,
			first_slot,
			bindings.data(),
			static_cast<uint32_t>(bindings.size())
		);
	}

	void RenderPass::bind_vertex_storage_textures(
		uint32_t first_slot,
		std::span<SDL_GPUTexture* const> textures
	) const noexcept
	{
		assert(resource != nullptr);
		SDL_BindGPUVertexStorageTextures(
			resource,
			first_slot,
			textures.data(),
			static_cast<uint32_t>(textures.size())
		);
	}

	void RenderPass::bind_vertex_storage_buffers(
		uint32_t first_slot,
		std::span<SDL_GPUBuffer* const> buffers
	) const noexcept
	{
		assert(resource != nullptr);
		SDL_BindGPUVertexStorageBuffers(
			resource,
			first_slot,
			buffers.data(),
			static_cast<uint32_t>(buffers.size())
		);
	}

	void RenderPass::bind_fragment_samplers(
		uint32_t first_slot,
		std::span<const SDL_GPUTextureSamplerBinding> bindings
	) const noexcept
	{
		assert(resource != nullptr);
		SDL_BindGPUFragmentSamplers(
			resource,
			first_slot,
			bindings.data(),
			static_cast<uint32_t>(bindings.size())
		);
	}

	void RenderPass::bind_fragment_storage_textures(
		uint32_t first_slot,
		std::span<SDL_GPUTexture* const> textures
	) const noexcept
	{
		assert(resource != nullptr);
		SDL_BindGPUFragmentStorageTextures(
			resource,
			first_slot,
			textures.data(),
			static_cast<uint32_t>(textures.size())
		);
	}

	void RenderPass::bind_fragment_storage_buffers(
		uint32_t first_slot,
		std::span<SDL_GPUBuffer* const> buffers
	) const noexcept
	{
		assert(resource != nullptr);
		SDL_BindGPUFragmentStorageBuffers(
			resource,
			first_slot,
			buffers.data(),
			static_cast<uint32_t>(buffers.size())
		);
	}

	void RenderPass::draw_indexed(
		uint32_t index_count,
		uint32_t index_offset,
		uint32_t instance_count,
		uint32_t instance_offset,
		int32_t vertex_offset
	) const noexcept
	{
		assert(resource != nullptr);
		SDL_DrawGPUIndexedPrimitives(
			resource,
			index_count,
			instance_count,
			index_offset,
			vertex_offset,
			instance_offset
		);
	}

	void RenderPass::draw(
		uint32_t vertex_count,
		uint32_t vertex_offset,
		uint32_t instance_count,
		uint32_t instance_offset
	) const noexcept
	{
		assert(resource != nullptr);
		SDL_DrawGPUPrimitives(resource, vertex_count, instance_count, vertex_offset, instance_offset);
	}

	void RenderPass::draw_indirect(const Buffer& buffer, uint32_t count, uint32_t offset) const noexcept
	{
		assert(resource != nullptr);
		SDL_DrawGPUPrimitivesIndirect(resource, buffer, offset, count);
	}

	void RenderPass::draw_indexed_indirect(
		const Buffer& buffer,
		uint32_t count,
		uint32_t offset
	) const noexcept
	{
		assert(resource != nullptr);
		SDL_DrawGPUIndexedPrimitivesIndirect(resource, buffer, offset, count);
	}

	void RenderPass::set_viewport(const SDL_GPUViewport& viewport) const noexcept
	{
		assert(resource != nullptr);
		SDL_SetGPUViewport(resource, &viewport);
	}

	void RenderPass::set_scissor(const SDL_Rect& scissor) const noexcept
	{
		assert(resource != nullptr);
		SDL_SetGPUScissor(resource, &scissor);
	}

	void RenderPass::set_stencil_reference(uint8_t reference) const noexcept
	{
		assert(resource != nullptr);
		SDL_SetGPUStencilReference(resource, reference);
	}

	void RenderPass::set_blend_constants(const SDL_FColor& blend_constants) const noexcept
	{
		assert(resource != nullptr);
		SDL_SetGPUBlendConstants(resource, blend_constants);
	}
}