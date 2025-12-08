#include "render/target/gbuffer.hpp"

namespace app::render::target
{
	std::expected<void, util::Error> Gbuffer::resize(SDL_GPUDevice* device, glm::u32vec2 size) noexcept
	{
		if (auto result = depth_texture.resize(device, size); !result) return result.error();
		if (auto result = albedo_texture.resize(device, size); !result) return result.error();
		if (auto result = lighting_info_texture.resize(device, size); !result) return result.error();
		return {};
	}
}
