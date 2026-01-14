#pragma once

#include <SDL3/SDL_gpu.h>
#include <expected>
#include <span>

#include "resource-box.hpp"
#include "util/error.hpp"

namespace gpu
{
	///
	/// @brief GPU Synchronization Fence
	///
	///
	class Fence : public ResourceBox<SDL_GPUFence>
	{
	  public:

		Fence(const Fence&) = delete;
		Fence(Fence&&) = default;
		Fence& operator=(const Fence&) = delete;
		Fence& operator=(Fence&&) = default;
		~Fence() noexcept = default;

		///
		/// @brief Query if the fence is signaled
		///
		/// @return `true` if signaled, `false` otherwise
		///
		bool is_signaled() const noexcept;

		///
		/// @brief Wait for the fence to be signaled
		///
		std::expected<void, util::Error> wait() const noexcept;

		///
		/// @brief Wait for any fence in the given list to be signaled
		///
		/// @param fences List of fences
		///
		static std::expected<void, util::Error> wait_any(
			SDL_GPUDevice* device,
			std::span<SDL_GPUFence* const> fences
		) noexcept;

		///
		/// @brief Wait for all fences in the given list to be signaled
		///
		/// @param fences List of fences
		///
		static std::expected<void, util::Error> wait_all(
			SDL_GPUDevice* device,
			std::span<SDL_GPUFence* const> fences
		) noexcept;

	  private:

		friend class CommandBuffer;

		using ResourceBox<SDL_GPUFence>::ResourceBox;
	};
}
