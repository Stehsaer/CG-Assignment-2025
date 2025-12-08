#include "gpu/command-buffer.hpp"
#include "gpu/util.hpp"

namespace gpu
{
	Command_buffer::Command_buffer(Command_buffer&& other) noexcept :
		device(std::exchange(other.device, nullptr)),
		cmd_buffer(std::exchange(other.cmd_buffer, nullptr))
	{}

	Command_buffer& Command_buffer::operator=(Command_buffer&& other) noexcept
	{
		if (&other != this)
		{
			this->~Command_buffer();
			new (this) Command_buffer(std::move(other));
		}

		return *this;
	}

	std::expected<Command_buffer, util::Error> Command_buffer::acquire_from(SDL_GPUDevice* device) noexcept
	{
		assert(device != nullptr);

		auto* const cmd_buffer = SDL_AcquireGPUCommandBuffer(device);
		if (cmd_buffer == nullptr) RETURN_SDL_ERROR;

		return Command_buffer(device, cmd_buffer);
	}

	std::expected<Copy_pass, util::Error> Command_buffer::begin_copy_pass() const noexcept
	{
		assert(cmd_buffer != nullptr);

		auto* const copy_pass = SDL_BeginGPUCopyPass(cmd_buffer);
		if (copy_pass == nullptr) RETURN_SDL_ERROR;

		return Copy_pass(copy_pass);
	}

	std::expected<void, util::Error> Command_buffer::run_copy_pass(
		const std::function<void(const Copy_pass&)>& task
	) const noexcept
	{
		assert(cmd_buffer != nullptr);

		auto command_buffer_result = this->begin_copy_pass();
		if (!command_buffer_result) return command_buffer_result.error();

		{
			auto& copy_pass = *command_buffer_result;
			task(copy_pass);
			copy_pass.end();
		}

		return {};
	}

	std::expected<Render_pass, util::Error> Command_buffer::begin_render_pass(
		std::span<const SDL_GPUColorTargetInfo> color_targets,
		const std::optional<SDL_GPUDepthStencilTargetInfo>& depth_stencil_target
	) const noexcept
	{
		assert(cmd_buffer != nullptr);

		auto* const render_pass = SDL_BeginGPURenderPass(
			cmd_buffer,
			color_targets.data(),
			color_targets.size(),
			depth_stencil_target.has_value() ? &depth_stencil_target.value() : nullptr
		);

		if (render_pass == nullptr) RETURN_SDL_ERROR;
		return Render_pass(render_pass);
	}

	std::expected<void, util::Error> Command_buffer::run_render_pass(
		std::span<const SDL_GPUColorTargetInfo> color_targets,
		const std::optional<SDL_GPUDepthStencilTargetInfo>& depth_stencil_target,
		const std::function<void(const Render_pass&)>& task
	) const noexcept
	{
		assert(cmd_buffer != nullptr);

		auto render_pass_result = this->begin_render_pass(color_targets, depth_stencil_target);
		if (!render_pass_result) return render_pass_result.error();

		{
			auto& render_pass = *render_pass_result;
			task(render_pass);
			render_pass.end();
		}

		return {};
	}

	std::expected<Compute_pass, util::Error> Command_buffer::begin_compute_pass(
		std::span<const SDL_GPUStorageTextureReadWriteBinding> storage_textures,
		std::span<const SDL_GPUStorageBufferReadWriteBinding> storage_buffers
	) const noexcept
	{
		assert(cmd_buffer != nullptr);

		auto* const compute_pass = SDL_BeginGPUComputePass(
			cmd_buffer,
			storage_textures.data(),
			static_cast<int>(storage_textures.size()),
			storage_buffers.data(),
			static_cast<int>(storage_buffers.size())
		);

		if (compute_pass == nullptr) RETURN_SDL_ERROR;
		return Compute_pass(compute_pass);
	}

	std::expected<void, util::Error> Command_buffer::run_compute_pass(
		std::span<const SDL_GPUStorageTextureReadWriteBinding> storage_textures,
		std::span<const SDL_GPUStorageBufferReadWriteBinding> storage_buffers,
		const std::function<void(const Compute_pass&)>& task
	) const noexcept
	{
		assert(cmd_buffer != nullptr);

		auto compute_pass_result = this->begin_compute_pass(storage_textures, storage_buffers);
		if (!compute_pass_result) return compute_pass_result.error();

		{
			auto& compute_pass = *compute_pass_result;
			task(compute_pass);
			compute_pass.end();
		}

		return {};
	}

	std::expected<std::optional<Command_buffer::Swapchain_texture_result>, util::Error>
	Command_buffer::acquire_swapchain_texture(SDL_Window* window) const noexcept
	{
		assert(cmd_buffer != nullptr);
		assert(window != nullptr);

		uint32_t width, height;
		SDL_GPUTexture* swapchain_texture;

		const auto success =
			SDL_AcquireGPUSwapchainTexture(cmd_buffer, window, &swapchain_texture, &width, &height);

		if (!success) RETURN_SDL_ERROR;

		if (swapchain_texture == nullptr) return std::nullopt;

		return Swapchain_texture_result{
			.swapchain_texture = swapchain_texture,
			.width = width,
			.height = height
		};
	}

	std::expected<Command_buffer::Swapchain_texture_result, util::Error>
	Command_buffer::wait_and_acquire_swapchain_texture(SDL_Window* window) const noexcept
	{
		assert(cmd_buffer != nullptr);
		assert(window != nullptr);

		uint32_t width, height;
		SDL_GPUTexture* swapchain_texture;

		const auto success =
			SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buffer, window, &swapchain_texture, &width, &height);

		if (!success) RETURN_SDL_ERROR;

		return Swapchain_texture_result{
			.swapchain_texture = swapchain_texture,
			.width = width,
			.height = height
		};
	}

	void Command_buffer::push_uniform_to_vertex(uint32_t slot, const void* data, size_t size) const noexcept
	{
		assert(cmd_buffer != nullptr);

		if (size == 0) return;
		SDL_PushGPUVertexUniformData(cmd_buffer, slot, data, static_cast<int>(size));
	}

	void Command_buffer::push_uniform_to_fragment(uint32_t slot, const void* data, size_t size) const noexcept
	{
		assert(cmd_buffer != nullptr);

		if (size == 0) return;
		SDL_PushGPUFragmentUniformData(cmd_buffer, slot, data, static_cast<int>(size));
	}

	void Command_buffer::push_uniform_to_compute(uint32_t slot, const void* data, size_t size) const noexcept
	{
		assert(cmd_buffer != nullptr);

		if (size == 0) return;
		SDL_PushGPUComputeUniformData(cmd_buffer, slot, data, static_cast<int>(size));
	}

	void Command_buffer::push_uniform_to_vertex(uint32_t slot, std::span<const std::byte> data) const noexcept
	{
		assert(cmd_buffer != nullptr);

		if (data.empty()) return;
		SDL_PushGPUVertexUniformData(cmd_buffer, slot, data.data(), static_cast<int>(data.size()));
	}

	void Command_buffer::push_uniform_to_fragment(
		uint32_t slot,
		std::span<const std::byte> data
	) const noexcept
	{
		assert(cmd_buffer != nullptr);

		if (data.empty()) return;
		SDL_PushGPUFragmentUniformData(cmd_buffer, slot, data.data(), static_cast<int>(data.size()));
	}

	void Command_buffer::push_uniform_to_compute(
		uint32_t slot,
		std::span<const std::byte> data
	) const noexcept
	{
		assert(cmd_buffer != nullptr);

		if (data.empty()) return;
		SDL_PushGPUComputeUniformData(cmd_buffer, slot, data.data(), static_cast<int>(data.size()));
	}

	void Command_buffer::generate_mipmaps(const Texture& texture) noexcept
	{
		assert(cmd_buffer != nullptr);
		SDL_GenerateMipmapsForGPUTexture(cmd_buffer, texture);
	}

	void Command_buffer::blit_texture(const SDL_GPUBlitInfo& blit_info) const noexcept
	{
		assert(cmd_buffer != nullptr);
		SDL_BlitGPUTexture(cmd_buffer, &blit_info);
	}

	void Command_buffer::insert_debug_label(const char* name) const noexcept
	{
		assert(cmd_buffer != nullptr);
		SDL_InsertGPUDebugLabel(cmd_buffer, name);
	}

	void Command_buffer::push_debug_group(const char* name) const noexcept
	{
		assert(cmd_buffer != nullptr);
		SDL_PushGPUDebugGroup(cmd_buffer, name);
	}

	void Command_buffer::pop_debug_group() const noexcept
	{
		assert(cmd_buffer != nullptr);
		SDL_PopGPUDebugGroup(cmd_buffer);
	}

	std::expected<void, util::Error> Command_buffer::submit() noexcept
	{
		assert(cmd_buffer != nullptr);

		if (!SDL_SubmitGPUCommandBuffer(cmd_buffer))
		{
			cmd_buffer = nullptr;
			device = nullptr;

			RETURN_SDL_ERROR;
		}

		cmd_buffer = nullptr;
		device = nullptr;

		return {};
	}

	std::expected<Fence, util::Error> Command_buffer::submit_and_acquire_fence() noexcept
	{
		assert(cmd_buffer != nullptr);

		auto* const fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd_buffer);
		if (fence == nullptr)
		{
			cmd_buffer = nullptr;
			device = nullptr;

			RETURN_SDL_ERROR;
		}

		cmd_buffer = nullptr;
		device = nullptr;

		return Fence(device, fence);
	}

	void Command_buffer::cancel() noexcept
	{
		assert(cmd_buffer != nullptr);
		SDL_CancelGPUCommandBuffer(cmd_buffer);
		cmd_buffer = nullptr;
		device = nullptr;
	}

	Command_buffer::operator SDL_GPUCommandBuffer*() const noexcept
	{
		return this->cmd_buffer;
	}

	Command_buffer::Command_buffer(SDL_GPUDevice* device, SDL_GPUCommandBuffer* resource) noexcept :
		device(device),
		cmd_buffer(resource)
	{}
}
