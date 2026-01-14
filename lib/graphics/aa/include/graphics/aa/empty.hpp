///
/// @file empty.hpp
/// @brief Defines an empty antialiasing processor that performs no antialiasing, only copying
///

#pragma once

#include "base.hpp"

namespace graphics::aa
{
	///
	/// @brief Empty Processor that only copies input to output
	///
	///
	class Empty : public Processor
	{
	  public:

		std::expected<void, util ::Error> run_antialiasing(
			SDL_GPUDevice* device,
			const gpu ::CommandBuffer& command_buffer,
			SDL_GPUTexture* source,
			SDL_GPUTexture* target,
			glm ::u32vec2 size
		) noexcept override;

		Empty(const Empty&) noexcept = delete;
		Empty& operator=(const Empty&) noexcept = delete;
		Empty(Empty&&) noexcept = default;
		Empty& operator=(Empty&&) noexcept = default;

		Empty() noexcept = default;
	};
}