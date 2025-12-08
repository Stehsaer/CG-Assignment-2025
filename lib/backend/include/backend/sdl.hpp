#pragma once

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <expected>
#include <glm/ext/vector_uint2_sized.hpp>
#include <memory>
#include <util/error.hpp>

namespace backend
{
	///
	/// @brief Initialize SDL Library
	///
	std::expected<void, util::Error> initialize_sdl();

	///
	/// @brief SDL Context, holding SDL Window and GPU Device
	///
	///
	class SDL_context
	{
	  public:

		SDL_context(SDL_Window* window, SDL_GPUDevice* device) noexcept;
		~SDL_context() noexcept = default;

		SDL_context(const SDL_context&) = delete;
		SDL_context(SDL_context&&) = delete;
		SDL_context& operator=(const SDL_context&) = delete;
		SDL_context& operator=(SDL_context&&) = delete;

		SDL_Window* const window;
		SDL_GPUDevice* const device;

		///
		/// @brief Create SDL Context with Window and GPU Device
		///
		/// @param width Window width
		/// @param height Window height
		/// @param title Title of the window
		/// @param additional_flags Additional SDL Window flags
		/// @param debug_enabled Enable debug layer for GPU Device
		/// @return `SDL_context` object on success, or `util::Error` on failure
		///
		static std::expected<std::unique_ptr<SDL_context>, util::Error> create(
			int width,
			int height,
			const std::string& title,
			SDL_WindowFlags additional_flags = 0,
			bool debug_enabled = false
		) noexcept;

		///
		/// @brief Get window scale factor
		///
		float get_window_scale() const noexcept;

		///
		/// @brief Get window size
		///
		glm::u32vec2 get_window_size() const noexcept;

		///
		/// @brief Get swapchain format
		///
		SDL_GPUTextureFormat get_swapchain_texture_format() const noexcept;
	};

}