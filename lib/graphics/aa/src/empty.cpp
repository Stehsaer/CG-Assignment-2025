#include "graphics/aa/empty.hpp"

namespace graphics::aa
{
	std::expected<void, util::Error> Empty::run_antialiasing(
		SDL_GPUDevice* device [[maybe_unused]],
		const gpu::CommandBuffer& command_buffer,
		SDL_GPUTexture* source,
		SDL_GPUTexture* target,
		glm::u32vec2 size
	) noexcept
	{
		const auto blit_info = SDL_GPUBlitInfo{
			.source =
				SDL_GPUBlitRegion{
								  .texture = source,
								  .mip_level = 0,
								  .layer_or_depth_plane = 0,
								  .x = 0,
								  .y = 0,
								  .w = size.x,
								  .h = size.y,
								  },
			.destination =
				SDL_GPUBlitRegion{
								  .texture = target,
								  .mip_level = 0,
								  .layer_or_depth_plane = 0,
								  .x = 0,
								  .y = 0,
								  .w = size.x,
								  .h = size.y,
								  },
			.load_op = SDL_GPU_LOADOP_DONT_CARE,
			.clear_color = {},
			.flip_mode = SDL_FLIP_NONE,
			.filter = SDL_GPU_FILTER_NEAREST,
			.cycle = true,
			.padding1 = 0,
			.padding2 = 0,
			.padding3 = 0
		};

		command_buffer.blit_texture(blit_info);

		return {};
	}
}