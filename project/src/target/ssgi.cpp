#include "target/ssgi.hpp"

namespace target
{
	std::expected<void, util::Error> SSGI::resize(SDL_GPUDevice* device, glm::u32vec2 size) noexcept
	{
		const auto half_size = (size + 1u) / 2u;

		if (const auto result = temporal_reservoir_texture1.resize_and_cycle(device, half_size); !result)
			return result.error().forward("Resize SSGI temporal reservoir texture 1 failed");
		if (const auto result = temporal_reservoir_texture2.resize_and_cycle(device, half_size); !result)
			return result.error().forward("Resize SSGI temporal reservoir texture 2 failed");
		if (const auto result = temporal_reservoir_texture3.resize_and_cycle(device, half_size); !result)
			return result.error().forward("Resize SSGI temporal reservoir texture 3 failed");
		if (const auto result = temporal_reservoir_texture4.resize_and_cycle(device, half_size); !result)
			return result.error().forward("Resize SSGI temporal reservoir texture 4 failed");

		if (const auto result = spatial_reservoir_texture1.resize_and_cycle(device, half_size); !result)
			return result.error().forward("Resize SSGI spatial reservoir texture 1 failed");
		if (const auto result = spatial_reservoir_texture2.resize_and_cycle(device, half_size); !result)
			return result.error().forward("Resize SSGI spatial reservoir texture 2 failed");
		if (const auto result = spatial_reservoir_texture3.resize_and_cycle(device, half_size); !result)
			return result.error().forward("Resize SSGI spatial reservoir texture 3 failed");
		if (const auto result = spatial_reservoir_texture4.resize_and_cycle(device, half_size); !result)
			return result.error().forward("Resize SSGI spatial reservoir texture 4 failed");

		if (const auto result = radiance_texture.resize_and_cycle(device, half_size); !result)
			return result.error().forward("Resize SSGI radiance texture failed");

		if (const auto result = fullres_radiance_texture.resize(device, size); !result)
			return result.error().forward("Resize SSGI fullres radiance texture failed");

		return {};
	}
}