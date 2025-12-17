#include "target/light.hpp"

namespace target
{
	std::expected<void, util::Error> Light_buffer::cycle(SDL_GPUDevice* device, glm::u32vec2 size) noexcept
	{
		if (auto result = light_texture.resize_and_cycle(device, size); !result)
			return result.error().forward("Resize light buffer texture failed");

		return {};
	}
}