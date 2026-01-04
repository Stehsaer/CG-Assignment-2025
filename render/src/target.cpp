#include "render/target.hpp"

namespace render
{
	std::expected<Target, util::Error> Target::create(SDL_GPUDevice* device) noexcept
	{
		auto auto_exposure_target = target::Auto_exposure::create(device);
		if (!auto_exposure_target)
			return auto_exposure_target.error().forward("Create auto exposure target failed");

		return Target{
			.gbuffer_target = {},
			.shadow_target = {},
			.light_buffer_target = {},
			.composite_target = {},
			.ao_target = {},
			.auto_exposure_target = std::move(*auto_exposure_target),
			.bloom_target = {},
			.ssgi_target = {}
		};
	}

	std::expected<void, util::Error> Target::resize_or_cycle(
		SDL_GPUDevice* device,
		glm::u32vec2 swapchain_size
	) noexcept
	{
		if (const auto result = gbuffer_target.cycle(device, swapchain_size); !result)
			return result.error().forward("Resize or cycle G-buffer target failed");

		if (const auto result = shadow_target.resize(device); !result)
			return result.error().forward("Resize shadow target failed");

		if (const auto result = light_buffer_target.cycle(device, swapchain_size); !result)
			return result.error().forward("Resize or cycle light buffer target failed");

		if (const auto result = composite_target.resize(device, swapchain_size); !result)
			return result.error().forward("Resize composite target failed");

		if (const auto result = ao_target.cycle(device, swapchain_size); !result)
			return result.error().forward("Resize or cycle AO target failed");

		auto_exposure_target.cycle();

		if (const auto result = bloom_target.resize(device, swapchain_size); !result)
			return result.error().forward("Resize bloom target failed");

		if (const auto result = ssgi_target.resize(device, swapchain_size); !result)
			return result.error().forward("Resize SSGI target failed");

		return {};
	}
}