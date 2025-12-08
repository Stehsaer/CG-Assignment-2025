#include "render/pass.hpp"

namespace app::render
{
	std::expected<gpu::Render_pass, util::Error> acquire_gbuffer_pass(
		const gpu::Command_buffer& command_buffer,
		const target::Gbuffer& gbuffer
	) noexcept
	{
		const auto albedo_target_info = SDL_GPUColorTargetInfo{
			.texture = *gbuffer.albedo_texture,
			.mip_level = 0,
			.layer_or_depth_plane = 0,
			.clear_color = {.r = 0, .g = 0, .b = 0, .a = 0},
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_STORE,
			.resolve_texture = nullptr,
			.resolve_mip_level = 0,
			.resolve_layer = 0,
			.cycle = true,
			.cycle_resolve_texture = false,
			.padding1 = 0,
			.padding2 = 0
		};

		const auto lighting_info_target_info = SDL_GPUColorTargetInfo{
			.texture = *gbuffer.lighting_info_texture,
			.mip_level = 0,
			.layer_or_depth_plane = 0,
			.clear_color = {.r = 0, .g = 0, .b = 0, .a = 0},
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_STORE,
			.resolve_texture = nullptr,
			.resolve_mip_level = 0,
			.resolve_layer = 0,
			.cycle = true,
			.cycle_resolve_texture = false,
			.padding1 = 0,
			.padding2 = 0
		};

		const auto depth_stencil_target_info = SDL_GPUDepthStencilTargetInfo{
			.texture = *gbuffer.depth_texture,
			.clear_depth = 0.0f,
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_STORE,
			.stencil_load_op = SDL_GPU_LOADOP_CLEAR,
			.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
			.cycle = true,
			.clear_stencil = 0,
			.padding1 = 0,
			.padding2 = 0
		};

		const std::array<SDL_GPUColorTargetInfo, 2> color_targets =
			{albedo_target_info, lighting_info_target_info};

		return command_buffer.begin_render_pass(color_targets, depth_stencil_target_info);
	}
}