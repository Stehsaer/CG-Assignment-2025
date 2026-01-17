#include "target/msaa-draw.hpp"
#include "util/error.hpp"

namespace target
{
	std::expected<void, util::Error> MSAADraw::resize(SDL_GPUDevice* device, glm::u32vec2 size) noexcept
	{
		if (const auto result = texture.resize(device, size); !result.has_value())
			return result.error().forward("Resize MSAA color texture failed");
		if (const auto result = depth_texture.resize(device, size); !result.has_value())
			return result.error().forward("Resize MSAA depth texture failed");

		return {};
	}
}