#include "graphics/util/buffer-pool.hpp"
#include "gpu/buffer.hpp"

namespace graphics
{
	void BufferPool::cycle() noexcept
	{
		for (auto& [key, buffer] : in_use_buffers) backup_pool[key].emplace_back(std::move(buffer));
		in_use_buffers.clear();
	}

	std::expected<std::shared_ptr<gpu::Buffer>, util::Error> BufferPool::acquire_buffer(
		gpu::Buffer::Usage usage,
		uint32_t size
	) noexcept
	{
		const PoolKey key{.size = size, .usage = usage};

		/* Search for available buffer */

		auto find_it = backup_pool.find(key);
		if (find_it != backup_pool.end() && !find_it->second.empty())  // Pool hit
		{
			auto buffer = std::move(find_it->second.back());
			find_it->second.pop_back();
			in_use_buffers.emplace_back(key, std::move(buffer));
			return in_use_buffers.back().second;
		}

		/* Create new buffer */

		auto buffer_result = gpu::Buffer::create(device, usage, size, "Pooled Buffer");
		if (!buffer_result) return buffer_result.error().forward("Create buffer failed");

		in_use_buffers.emplace_back(key, std::make_shared<gpu::Buffer>(std::move(buffer_result.value())));
		return in_use_buffers.back().second;
	}

	void BufferPool::gc() noexcept
	{
		backup_pool.clear();
	}

	void TransferBufferPool::cycle() noexcept
	{
		for (auto& [key, buffer] : in_use_buffers) backup_pool[key].emplace_back(std::move(buffer));
		in_use_buffers.clear();
	}

	std::expected<std::shared_ptr<gpu::TransferBuffer>, util::Error> TransferBufferPool::acquire_buffer(
		gpu::TransferBuffer::Usage usage,
		uint32_t size
	) noexcept
	{
		const PoolKey key{.usage = usage, .size = size};

		/* Search for available buffer */

		auto find_it = backup_pool.find(key);
		if (find_it != backup_pool.end() && !find_it->second.empty())  // Pool hit
		{
			auto buffer = std::move(find_it->second.back());
			find_it->second.pop_back();
			in_use_buffers.emplace_back(key, std::move(buffer));
			return in_use_buffers.back().second;
		}

		/* Create new buffer */

		auto buffer_result = gpu::TransferBuffer::create(device, usage, size);
		if (!buffer_result) return buffer_result.error().forward("Create transfer buffer failed");

		in_use_buffers
			.emplace_back(key, std::make_shared<gpu::TransferBuffer>(std::move(buffer_result.value())));
		return in_use_buffers.back().second;
	}

	void TransferBufferPool::gc() noexcept
	{
		backup_pool.clear();
	}
}