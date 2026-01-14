#pragma once

#include "compute-pass.hpp"
#include "copy-pass.hpp"
#include "fence.hpp"
#include "render-pass.hpp"
#include "texture.hpp"

#include <SDL3/SDL_gpu.h>
#include <expected>
#include <optional>
#include <span>

namespace gpu
{
	///
	/// @brief Command buffer
	/// @note Acquire the command buffer from a GPU device using @p acquire_from()
	/// @note No automatic submission, explicitly submit using @p submit()
	///
	class CommandBuffer
	{
	  public:

		CommandBuffer(const CommandBuffer&) = delete;
		CommandBuffer& operator=(const CommandBuffer&) = delete;

		CommandBuffer(CommandBuffer&&) noexcept;
		CommandBuffer& operator=(CommandBuffer&&) noexcept;

		~CommandBuffer() noexcept = default;

		///
		/// @brief Acquire a command buffer from a GPU device
		///
		/// @return Command buffer object, or error if failed
		///
		static std::expected<CommandBuffer, util::Error> acquire_from(SDL_GPUDevice* device) noexcept;

		///
		/// @brief Begins a new copy pass
		///
		/// @return Copy pass object, or error if failed
		///
		std::expected<CopyPass, util::Error> begin_copy_pass() const noexcept;

		///
		/// @brief Runs a new copy pass
		///
		/// @param task Task function for the copy pass
		///
		std::expected<void, util::Error> run_copy_pass(
			const std::function<void(const CopyPass&)>& task
		) const noexcept;

		///
		/// @brief Begins a new render pass
		///
		/// @param color_targets Color buffer targets
		/// @param depth_stencil_target Depth stencil buffer target
		/// @return Render pass object, or error if failed
		///
		std::expected<RenderPass, util::Error> begin_render_pass(
			std::span<const SDL_GPUColorTargetInfo> color_targets,
			const std::optional<SDL_GPUDepthStencilTargetInfo>& depth_stencil_target
		) const noexcept;

		///
		/// @brief Runs a new render pass
		///
		/// @param color_targets Color buffer targets
		/// @param depth_stencil_target Depth stencil buffer target
		/// @param task Render task function
		///
		std::expected<void, util::Error> run_render_pass(
			std::span<const SDL_GPUColorTargetInfo> color_targets,
			const std::optional<SDL_GPUDepthStencilTargetInfo>& depth_stencil_target,
			const std::function<void(const RenderPass&)>& task
		) const noexcept;

		///
		/// @brief Begins a new compute pass
		///
		/// @param storage_textures Storage textures
		/// @param storage_buffers Storage buffers
		/// @return Compute pass object, or error if failed
		///
		std::expected<ComputePass, util::Error> begin_compute_pass(
			std::span<const SDL_GPUStorageTextureReadWriteBinding> storage_textures,
			std::span<const SDL_GPUStorageBufferReadWriteBinding> storage_buffers
		) const noexcept;

		///
		/// @brief Runs a new compute pass
		///
		/// @param storage_textures Storage textures
		/// @param storage_buffers Storage buffers
		/// @param task Compute task function
		///
		std::expected<void, util::Error> run_compute_pass(
			std::span<const SDL_GPUStorageTextureReadWriteBinding> storage_textures,
			std::span<const SDL_GPUStorageBufferReadWriteBinding> storage_buffers,
			const std::function<void(const ComputePass&)>& task
		) const noexcept;

		struct SwapchainTextureResult
		{
			SDL_GPUTexture* swapchain_texture;
			uint32_t width;
			uint32_t height;
		};

		///
		/// @brief Acquires a new swapchain texture
		///
		/// @param window Associated window
		/// @return Swapchain texture and its dimensions, or error if failed
		///
		std::expected<std::optional<SwapchainTextureResult>, util::Error> acquire_swapchain_texture(
			SDL_Window* window
		) const noexcept;

		///
		/// @brief Waits for and acquires a new swapchain texture
		///
		/// @param window Associated window
		/// @return Swapchain texture and its dimensions, or error if failed
		///
		std::expected<SwapchainTextureResult, util::Error> wait_and_acquire_swapchain_texture(
			SDL_Window* window
		) const noexcept;

		///
		/// @brief Pushes uniform data to the vertex shader
		///
		/// @param slot Slot
		/// @param data Raw data pointer
		/// @param size Data size in bytes
		///
		void push_uniform_to_vertex(uint32_t slot, const void* data, size_t size) const noexcept;

		///
		/// @brief Pushes uniform data to the vertex shader
		///
		/// @param slot Slot
		/// @param data Data byte span
		///
		void push_uniform_to_vertex(uint32_t slot, std::span<const std::byte> data) const noexcept;

		///
		/// @brief Pushes uniform data to the fragment shader
		///
		/// @param slot Slot
		/// @param data Raw data pointer
		/// @param size Data size in bytes
		///
		void push_uniform_to_fragment(uint32_t slot, const void* data, size_t size) const noexcept;

		///
		/// @brief Pushes uniform data to the fragment shader
		///
		/// @param slot Slot
		/// @param data Data byte span
		///
		void push_uniform_to_fragment(uint32_t slot, std::span<const std::byte> data) const noexcept;

		///
		/// @brief Pushes uniform data to the compute shader
		///
		/// @param slot Slot
		/// @param data Raw data pointer
		/// @param size Data size in bytes
		///
		void push_uniform_to_compute(uint32_t slot, const void* data, size_t size) const noexcept;

		///
		/// @brief Pushes uniform data to the compute shader
		///
		/// @param slot Slot
		/// @param data Data byte span
		///
		void push_uniform_to_compute(uint32_t slot, std::span<const std::byte> data) const noexcept;

		///
		/// @brief Generates mipmaps for a texture
		///
		/// @param texture Texture
		///
		void generate_mipmaps(const Texture& texture) noexcept;

		///
		/// @brief Blits a texture
		///
		/// @param blit_info Operation information
		///
		void blit_texture(const SDL_GPUBlitInfo& blit_info) const noexcept;

		///
		/// @brief Inserts a debug label into the command buffer
		///
		/// @param name Label name
		///
		void insert_debug_label(const char* name) const noexcept;

		///
		/// @brief Begins a debug group in the command buffer
		///
		/// @param name Debug group name
		///
		void push_debug_group(const char* name) const noexcept;

		///
		/// @brief Ends the current debug group in the command buffer
		///
		///
		void pop_debug_group() const noexcept;

		///
		/// @brief Explicitly submits the command buffer
		///
		///
		std::expected<void, util::Error> submit() noexcept;

		///
		/// @brief Submits the command buffer and acquires a fence
		///
		/// @return Fence object, or error if failed
		///
		std::expected<Fence, util::Error> submit_and_acquire_fence() noexcept;

		///
		/// @brief Cancels and releases the command buffer
		///
		///
		void cancel() noexcept;

		operator SDL_GPUCommandBuffer*() const noexcept;

	  private:

		CommandBuffer(SDL_GPUDevice* device, SDL_GPUCommandBuffer* resource) noexcept;

		SDL_GPUDevice* device;
		SDL_GPUCommandBuffer* cmd_buffer;
	};
}