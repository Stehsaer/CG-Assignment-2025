#include "render/pass.hpp"
#include "gpu/render-pass.hpp"

#include <SDL3/SDL_gpu.h>
#include <array>

namespace render
{
	std::expected<gpu::Render_pass, util::Error> acquire_gbuffer_pass(
		const gpu::Command_buffer& command_buffer,
		const target::Gbuffer& gbuffer,
		const target::Light_buffer& light_buffer
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

		const auto light_target_info = SDL_GPUColorTargetInfo{
			.texture = light_buffer.light_texture.current(),
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
			.stencil_store_op = SDL_GPU_STOREOP_STORE,
			.cycle = true,
			.clear_stencil = 0,
			.mip_level = 0,
			.layer = 0
		};

		const std::array color_targets = {albedo_target_info, lighting_info_target_info, light_target_info};

		return command_buffer.begin_render_pass(color_targets, depth_stencil_target_info);
	}

	std::expected<gpu::Render_pass, util::Error> acquire_lighting_pass(
		const gpu::Command_buffer& command_buffer,
		const target::Light_buffer& light_buffer,
		const target::Gbuffer& gbuffer
	) noexcept
	{
		const auto light_target_info = SDL_GPUColorTargetInfo{
			.texture = light_buffer.light_texture.current(),
			.mip_level = 0,
			.layer_or_depth_plane = 0,
			.clear_color = {.r = 0, .g = 0, .b = 0, .a = 0},
			.load_op = SDL_GPU_LOADOP_LOAD,
			.store_op = SDL_GPU_STOREOP_STORE,
			.resolve_texture = nullptr,
			.resolve_mip_level = 0,
			.resolve_layer = 0,
			.cycle = false,
			.cycle_resolve_texture = false,
			.padding1 = 0,
			.padding2 = 0
		};

		const std::array color_targets = {light_target_info};

		const auto depth_stencil_target_info = SDL_GPUDepthStencilTargetInfo{
			.texture = *gbuffer.depth_texture,
			.clear_depth = 0.0f,
			.load_op = SDL_GPU_LOADOP_LOAD,
			.store_op = SDL_GPU_STOREOP_STORE,
			.stencil_load_op = SDL_GPU_LOADOP_LOAD,
			.stencil_store_op = SDL_GPU_STOREOP_STORE,
			.cycle = false,
			.clear_stencil = 0,
			.mip_level = 0,
			.layer = 0
		};

		return command_buffer.begin_render_pass(color_targets, depth_stencil_target_info);
	}

	std::expected<gpu::Render_pass, util::Error> acquire_shadow_pass(
		const gpu::Command_buffer& command_buffer,
		const target::Shadow& shadow_target,
		size_t level
	) noexcept
	{
		SDL_GPUTexture* depth_texture;
		switch (level)
		{
		case 0:
			depth_texture = *shadow_target.depth_texture_level0;
			break;
		case 1:
			depth_texture = *shadow_target.depth_texture_level1;
			break;
		case 2:
			depth_texture = *shadow_target.depth_texture_level2;
			break;
		default:
			return util::Error("Invalid CSM level index");
		}

		const auto depth_stencil_target_info = SDL_GPUDepthStencilTargetInfo{
			.texture = depth_texture,
			.clear_depth = 0.0f,
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_STORE,
			.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
			.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
			.cycle = true,
			.clear_stencil = 0,
			.mip_level = 0,
			.layer = 0
		};

		return command_buffer.begin_render_pass({}, depth_stencil_target_info);
	}

	std::expected<gpu::Render_pass, util::Error> acquire_ao_pass(
		const gpu::Command_buffer& command_buffer,
		const target::AO& ao_target
	) noexcept
	{
		const auto ao_target_info = SDL_GPUColorTargetInfo{
			.texture = ao_target.halfres_ao_texture.current(),
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

		const std::array color_targets = {ao_target_info};

		return command_buffer.begin_render_pass(color_targets, std::nullopt);
	}

	std::expected<gpu::Render_pass, util::Error> acquire_swapchain_pass(
		const gpu::Command_buffer& command_buffer,
		SDL_GPUTexture* swapchain_texture,
		bool clear
	) noexcept
	{
		const auto swapchain_target_info = SDL_GPUColorTargetInfo{
			.texture = swapchain_texture,
			.mip_level = 0,
			.layer_or_depth_plane = 0,
			.clear_color = {.r = 0, .g = 0, .b = 0, .a = 1},
			.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD,
			.store_op = SDL_GPU_STOREOP_STORE,
			.resolve_texture = nullptr,
			.resolve_mip_level = 0,
			.resolve_layer = 0,
			.cycle = clear,
			.cycle_resolve_texture = false,
			.padding1 = 0,
			.padding2 = 0
		};

		const std::array color_targets = std::to_array({swapchain_target_info});

		return command_buffer.begin_render_pass(color_targets, std::nullopt);
	}
}