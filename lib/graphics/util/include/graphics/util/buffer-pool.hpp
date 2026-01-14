#pragma once

#include <SDL3/SDL_gpu.h>
#include <gpu/buffer.hpp>
#include <map>
#include <memory>
#include <vector>

namespace graphics
{
	class BufferPool
	{
		struct PoolKey
		{
			uint32_t size;
			gpu::Buffer::Usage usage;

			auto operator<=>(const PoolKey&) const = default;
			bool operator==(const PoolKey&) const = default;
		};

		std::map<PoolKey, std::vector<std::shared_ptr<gpu::Buffer>>> backup_pool;
		std::vector<std::pair<PoolKey, std::shared_ptr<gpu::Buffer>>> in_use_buffers;

		SDL_GPUDevice* device;

	  public:

		BufferPool(SDL_GPUDevice* device) noexcept :
			device(device)
		{}

		///
		/// @brief Return all in-use buffers to the pool. Called before a new frame
		/// @note After calling this, all previously obtained buffer references should be treated as
		/// invalidated, as some of them may be cleaned up and reused
		/// @warning Not thread-safe
		///
		void cycle() noexcept;

		///
		/// @brief Acquire a buffer by size and usage
		/// @note Must be called after `cycle()`
		/// @warning Not thread-safe
		///
		/// @param usage Usage of the buffer
		/// @param size Size of the buffer in size
		/// @return Acquired buffer, or error if failed
		///
		std::expected<std::shared_ptr<gpu::Buffer>, util::Error> acquire_buffer(
			gpu::Buffer::Usage usage,
			uint32_t size
		) noexcept;

		///
		/// @brief Recycle unused buffers from the pool to free memory
		/// @note Should be called after all `acquire_buffer` in a frame
		/// @warning Not thread-safe
		///
		void gc() noexcept;

		BufferPool(const BufferPool&) = delete;
		BufferPool(BufferPool&&) = default;
		BufferPool& operator=(const BufferPool&) = delete;
		BufferPool& operator=(BufferPool&&) = default;
	};

	class TransferBufferPool
	{
		struct PoolKey
		{
			gpu::TransferBuffer::Usage usage;
			uint32_t size;

			auto operator<=>(const PoolKey&) const = default;
			bool operator==(const PoolKey&) const = default;
		};

		std::map<PoolKey, std::vector<std::shared_ptr<gpu::TransferBuffer>>> backup_pool;
		std::vector<std::pair<PoolKey, std::shared_ptr<gpu::TransferBuffer>>> in_use_buffers;

		SDL_GPUDevice* device;

	  public:

		TransferBufferPool(SDL_GPUDevice* device) noexcept :
			device(device)
		{}

		///
		/// @brief Return all in-use buffers to the pool. Called before a new frame
		/// @note After calling this, all previously obtained buffer references should be treated as
		/// invalidated, as some of them may be cleaned up and reused
		/// @warning Not thread-safe
		///
		void cycle() noexcept;

		///
		/// @brief Acquire a transfer buffer by size and usage
		/// @note Must be called after `cycle()`
		/// @warning Not thread-safe
		///
		/// @param usage Usage of the buffer
		/// @param size Size of the buffer in size
		/// @return Acquired buffer, or error if failed
		///
		std::expected<std::shared_ptr<gpu::TransferBuffer>, util::Error> acquire_buffer(
			gpu::TransferBuffer::Usage usage,
			uint32_t size
		) noexcept;

		///
		/// @brief Recycle unused buffers from the pool to free memory
		/// @note Should be called after all `acquire_buffer` in a frame
		/// @warning Not thread-safe
		///
		void gc() noexcept;

		TransferBufferPool(const TransferBufferPool&) = delete;
		TransferBufferPool(TransferBufferPool&&) = default;
		TransferBufferPool& operator=(const TransferBufferPool&) = delete;
		TransferBufferPool& operator=(TransferBufferPool&&) = default;
	};
}