#include "backend/sdl.hpp"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>
#include <format>
#include <glm/ext/vector_int2.hpp>
#include <vulkan/vulkan.h>

namespace backend
{
	struct SDLCleanup
	{
		~SDLCleanup() noexcept { SDL_Quit(); }
	};

	static std::unique_ptr<SDLCleanup> sdl_cleanup_instance = nullptr;

	std::expected<void, util::Error> initialize_sdl()
	{
		if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
			return util::Error(std::format("Initialize SDL failed: {}", SDL_GetError()));

		sdl_cleanup_instance = std::make_unique<SDLCleanup>();

		return {};
	}

	static std::expected<SDL_GPUDevice*, util::Error> create_gpu(const VulkanConfig& vk_config) noexcept
	{
		SDL_GPUVulkanOptions vulkan_options{
			.vulkan_api_version = VK_MAKE_VERSION(
				vk_config.vulkan_version_major,
				vk_config.vulkan_version_minor,
				vk_config.vulkan_version_patch
			),
			.feature_list = nullptr,
			.vulkan_10_physical_device_features = nullptr,
			.device_extension_count = 0,
			.device_extension_names = nullptr,
			.instance_extension_count = 0,
			.instance_extension_names = nullptr
		};

		const SDL_PropertiesID device_creation_id = SDL_CreateProperties();
		SDL_SetBooleanProperty(device_creation_id, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
		SDL_SetBooleanProperty(
			device_creation_id,
			SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN,
			vk_config.debug_enabled
		);
		SDL_SetStringProperty(device_creation_id, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, "vulkan");
		SDL_SetPointerProperty(
			device_creation_id,
			SDL_PROP_GPU_DEVICE_CREATE_VULKAN_OPTIONS_POINTER,
			&vulkan_options
		);

		/* Create Device */

		SDL_GPUDevice* const gpu_device = SDL_CreateGPUDeviceWithProperties(device_creation_id);
		if (gpu_device == nullptr)
		{
			return util::Error(std::format("Create SDL GPU failed: {}", SDL_GetError()));
		}

		SDL_DestroyProperties(device_creation_id);

		return gpu_device;
	}

	std::expected<std::unique_ptr<SDLcontext>, util::Error> SDLcontext::create(
		int width,
		int height,
		const std::string& title,
		SDL_WindowFlags additional_flags,
		const VulkanConfig& vk_config
	) noexcept
	{
		/* Create Window */

		SDL_Window* const window =
			SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_HIGH_PIXEL_DENSITY | additional_flags);
		if (window == nullptr)
			return util::Error(std::format("Create SDL Window failed: {}", SDL_GetError()));

		/* Prepare Device Creation Info */

		auto gpu_result = create_gpu(vk_config);
		if (!gpu_result)
		{
			SDL_DestroyWindow(window);
			return gpu_result.error().forward("Create SDL GPU Device failed");
		}

		SDL_GPUDevice* const gpu_device = *gpu_result;

		/* Claim Window */

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

		return std::make_unique<SDLcontext>(window, gpu_device);
	}

	float SDLcontext::get_window_scale() const noexcept
	{
		return SDL_GetDisplayContentScale(SDL_GetDisplayForWindow(window));
	}

	glm::u32vec2 SDLcontext::get_window_size() const noexcept
	{
		glm::ivec2 size;
		SDL_GetWindowSize(window, &size.x, &size.y);
		return size;
	}

	SDL_GPUTextureFormat SDLcontext::get_swapchain_texture_format() const noexcept
	{
		return SDL_GetGPUSwapchainTextureFormat(device, window);
	}

	SDLcontext::~SDLcontext() noexcept
	{
		SDL_WaitForGPUSwapchain(device, window);
		SDL_WaitForGPUIdle(device);
		SDL_DestroyGPUDevice(device);
		SDL_DestroyWindow(window);
	}
}