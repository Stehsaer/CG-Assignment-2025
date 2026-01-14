#pragma once

#include "gpu/texture.hpp"

#include "util/error.hpp"
#include <glm/glm.hpp>
#include <memory>

namespace graphics
{
	///
	/// @brief Smart texture, provides resize functionality
	/// @details #### Usage:
	/// - Assigns a format at construction
	/// - Resize using `resize` when needed, eg. after acquiring the swapchain
	/// - Use `operator*` to get the underlying image reference, valid until the next resize
	/// @note 2D, single-layer mipmap textures only
	///
	class AutoTexture
	{
	  public:

		AutoTexture(const AutoTexture&) = delete;
		AutoTexture& operator=(const AutoTexture&) = delete;
		AutoTexture(AutoTexture&&) = default;
		AutoTexture& operator=(AutoTexture&&) = default;

		AutoTexture(gpu::Texture::Format format, std::string name, uint32_t mip_levels = 1) noexcept :
			format(format),
			name(std::move(name)),
			mip_levels(mip_levels)
		{}

		~AutoTexture() = default;

		///
		/// @brief Resize the texture
		///
		/// @param size New size
		///
		std::expected<void, util::Error> resize(SDL_GPUDevice* device, glm::u32vec2 size) noexcept;

		///
		/// @brief Get the size of the texture
		///
		/// @return Current size
		///
		glm::u32vec2 get_size() const noexcept;

		const gpu::Texture& operator*() const noexcept;

		const gpu::Texture* operator->() const noexcept;

	  private:

		gpu::Texture::Format format;
		std::string name;

		glm::u32vec2 size = {0, 0};
		uint32_t mip_levels = 1;
		std::unique_ptr<gpu::Texture> texture;
	};

	///
	/// @brief Smart texture that cycles between multiple textures each frame, to store previous frame data
	/// @note Follows similar API to @p Smart_texture
	///
	class CycleTexture
	{
	  public:

		CycleTexture(const CycleTexture&) = delete;
		CycleTexture(CycleTexture&&) noexcept = default;
		CycleTexture& operator=(const CycleTexture&) = delete;
		CycleTexture& operator=(CycleTexture&&) = default;

		///
		/// @brief Construct a Cycle_texture
		///
		/// @param format Format of the texture
		/// @param extra_pool_size Extra pool size, besides current and previous frame textures
		///
		CycleTexture(
			gpu::Texture::Format format,
			std::string name,
			uint32_t mip_levels = 1,
			size_t extra_pool_size = 1
		) noexcept :
			format(format),
			name(std::move(name)),
			extra_pool_size(std::max(1zu, extra_pool_size)),
			mip_levels(mip_levels)
		{}

		///
		/// @brief Resize the texture, and advance to the next texture in the cycle
		/// @note This invalidates any previously obtained texture pointers
		///
		/// @param size New size
		///
		std::expected<void, util::Error> resize_and_cycle(
			SDL_GPUDevice* device,
			glm::u32vec2 new_size
		) noexcept;

		///
		/// @brief Get current size of the textures
		///
		/// @return Current size
		///
		glm::u32vec2 get_size() const noexcept;

		///
		/// @brief Get the current frame texture
		///
		/// @return Current frame texture pointer
		///
		const gpu::Texture& current() const noexcept;

		///
		/// @brief Get the previous frame texture
		///
		/// @return Previous frame texture pointer
		///
		const gpu::Texture& prev() const noexcept;

	  private:

		gpu::Texture::Format format;
		std::string name;

		glm::u32vec2 size = {0, 0};
		size_t extra_pool_size = 1;
		uint32_t mip_levels = 1;

		std::vector<std::unique_ptr<gpu::Texture>> texture_pool;
	};
}