#include "target/msaa-draw.hpp"
#include "util/error.hpp"

namespace target
{
	std::expected<void, util::Error> MSAADraw::resize(SDL_GPUDevice* device, glm::u32vec2 size) noexcept
	{
		return texture.resize(device, size)
			.transform_error(util::Error::forward_fn("Resize MSAA texture failed"));
	}
}