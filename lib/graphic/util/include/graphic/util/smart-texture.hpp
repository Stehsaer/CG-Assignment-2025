#pragma once

#include <glm/glm.hpp>
#include <gpu.hpp>
#include <memory>
#include <util/error.hpp>

namespace graphic
{
	///
	/// @brief Smart texture, provides resize functionality
	/// @details #### Usage:
	/// - Assigns a format at construction
	/// - Resize using `resize` when needed, eg. after acquiring the swapchain
	/// - Use `operator*` to get the underlying SDL_GPUTexture pointer, valid until the next resize
	/// @note 2D, single-layer mipmap textures only
	///
	class Smart_texture
	{
	  public:

		Smart_texture(const Smart_texture&) = delete;
		Smart_texture& operator=(const Smart_texture&) = delete;
		Smart_texture(Smart_texture&&) = default;
		Smart_texture& operator=(Smart_texture&&) = default;

		Smart_texture(gpu::Texture::Format format) noexcept;
		~Smart_texture() = default;

		///
		/// @brief Resize the texture
		/// @note This invalidates any previously obtained texture pointers
		///
		/// @param size New size
		///
		std::expected<void, util::Error> resize(SDL_GPUDevice* device, glm::u32vec2 size) noexcept;

		///
		/// @brief Get the texture pointer
		/// @note
		/// - The acquired pointer will be invalidated upon the next resize
		/// - `resize` must be called at least once before getting the pointer
		///
		SDL_GPUTexture* operator*() const noexcept;

	  private:

		glm::u32vec2 size = {0, 0};
		gpu::Texture::Format format;
		std::unique_ptr<gpu::Texture> texture;
	};
}