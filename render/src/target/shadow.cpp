#include "render/target/shadow.hpp"
#include "render/const-params.hpp"

namespace render::target
{
	std::expected<void, util::Error> Shadow::resize(SDL_GPUDevice* device) noexcept
	{
		if (auto result = depth_texture_level0.resize(device, glm::u32vec2(SHADOW_LEVEL_RES_0)); !result)
			return result.error().forward("Resize Shadow depth texture failed");

		if (auto result = depth_texture_level1.resize(device, glm::u32vec2(SHADOW_LEVEL_RES_1)); !result)
			return result.error().forward("Resize Shadow depth texture level 1 failed");

		if (auto result = depth_texture_level2.resize(device, glm::u32vec2(SHADOW_LEVEL_RES_2)); !result)
			return result.error().forward("Resize Shadow depth texture level 2 failed");

		return {};
	}
}