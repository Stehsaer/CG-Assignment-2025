///
/// @file sdl.hpp
/// @brief Provide SDL initialization and context management class
///
///

#pragma once

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <expected>
#include <glm/ext/vector_uint2_sized.hpp>
#include <memory>

#include "util/error.hpp"

namespace backend
{
	///
	/// @brief Initialize SDL Library
	/// @note Call this function before using any SDL functionality.
	///
	std::expected<void, util::Error> initialize_sdl();

	struct VulkanConfig
	{
		bool debug_enabled = false;
		uint8_t vulkan_version_major = 1;
		uint8_t vulkan_version_minor = 3;
		uint8_t vulkan_version_patch = 0;
	};

	///
	/// @brief SDL Context, holds SDL Window and GPU Device and provides helper functions
	///
	///
	class SDLcontext
	{
	  public:

		SDLcontext(SDL_Window* window, SDL_GPUDevice* device) noexcept :
			window(window),
			device(device)
		{}

		~SDLcontext() noexcept;

		SDLcontext(const SDLcontext&) = delete;
		SDLcontext(SDLcontext&&) = delete;
		SDLcontext& operator=(const SDLcontext&) = delete;
		SDLcontext& operator=(SDLcontext&&) = delete;

		SDL_Window* const window;
		SDL_GPUDevice* const device;

		///
		/// @brief Create SDL Context with Window and GPU Device
		/// @note Call `initialize_sdl` before calling this function.
		///
		/// @param width Window width
		/// @param height Window height
		/// @param title Title of the window
		/// @param additional_flags Additional SDL Window flags
		/// @param debug_enabled Enable debug layer for GPU Device
		/// @return `SDL_context` object on success, or `util::Error` on failure
		///
		static std::expected<std::unique_ptr<SDLcontext>, util::Error> create(
			int width,
			int height,
			const std::string& title,
			SDL_WindowFlags additional_flags = 0,
			const VulkanConfig& vk_config = {}
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