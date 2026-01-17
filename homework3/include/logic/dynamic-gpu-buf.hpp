#pragma once

#include "gpu/copy-pass.hpp"

#include <gpu/buffer.hpp>
#include <span>

namespace logic
{
	class DynamicGPUBuffer
	{
		const bool usage_vertex;
		const bool usage_indirect;

		size_t capacity = 0;
		size_t size = 0;
		bool buffers_synchronized = false;

		std::optional<gpu::TransferBuffer> transfer_buffer = std::nullopt;
		std::optional<gpu::Buffer> buffer = std::nullopt;

	  public:

		DynamicGPUBuffer(bool usage_vertex, bool usage_indirect) :
			usage_vertex(usage_vertex),
			usage_indirect(usage_indirect)
		{}

		void copy_to_gpu(const gpu::CopyPass& copy_pass) noexcept;

		std::expected<void, util::Error> write_transfer(
			SDL_GPUDevice* device,
			std::span<const std::byte> data
		) noexcept;

		std::optional<SDL_GPUBuffer*> get_buffer() const noexcept { return buffer; }
	};
}