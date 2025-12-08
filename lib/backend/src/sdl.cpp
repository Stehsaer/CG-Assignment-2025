#include "backend/sdl.hpp"

#include <SDL3/SDL_init.h>
#include <format>
#include <glm/ext/vector_int2.hpp>

namespace backend
{
	struct SDL_cleanup
	{
		~SDL_cleanup() noexcept { SDL_Quit(); }
	};

	static std::unique_ptr<SDL_cleanup> sdl_cleanup_instance = nullptr;

	std::expected<void, util::Error> initialize_sdl()
	{
		if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
			return util::Error(std::format("初始化 SDL 失败: {}", SDL_GetError()));

		sdl_cleanup_instance = std::make_unique<SDL_cleanup>();

		return {};
	}

	SDL_context::SDL_context(SDL_Window* window, SDL_GPUDevice* device) noexcept :
		window(window),
		device(device)
	{}

	std::expected<std::unique_ptr<SDL_context>, util::Error> SDL_context::create(
		int width,
		int height,
		const std::string& title,
		SDL_WindowFlags additional_flags,
		bool debug_enabled
	) noexcept
	{
		SDL_Window* const window =
			SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_HIGH_PIXEL_DENSITY | additional_flags);
		if (window == nullptr)
			return util::Error(std::format("Create SDL Window failed: {}", SDL_GetError()));

		SDL_GPUDevice* const gpu_device =
			SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, debug_enabled, "vulkan");
		if (gpu_device == nullptr)
		{
			SDL_DestroyWindow(window);
			return util::Error(std::format("Create SDL GPU failed: {}", SDL_GetError()));
		}

		if (!SDL_ClaimWindowForGPUDevice(gpu_device, window))
		{
			SDL_DestroyGPUDevice(gpu_device);
			SDL_DestroyWindow(window);
			return util::Error(std::format("Claim SDL Window for SDL GPU Device Failed: {}", SDL_GetError()));
		}

		if (!SDL_SetGPUSwapchainParameters(
				gpu_device,
				window,
				SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
				SDL_GPU_PRESENTMODE_VSYNC
			))
		{
			SDL_DestroyGPUDevice(gpu_device);
			SDL_DestroyWindow(window);
			return util::Error(std::format("Set SDL Swapchain Parameters Failed: {}", SDL_GetError()));
		}

		return std::make_unique<SDL_context>(window, gpu_device);
	}

	float SDL_context::get_window_scale() const noexcept
	{
		return SDL_GetDisplayContentScale(SDL_GetDisplayForWindow(window));
	}

	glm::u32vec2 SDL_context::get_window_size() const noexcept
	{
		glm::ivec2 size;
		SDL_GetWindowSize(window, &size.x, &size.y);
		return size;
	}

	SDL_GPUTextureFormat SDL_context::get_swapchain_texture_format() const noexcept
	{
		return SDL_GetGPUSwapchainTextureFormat(device, window);
	}
}