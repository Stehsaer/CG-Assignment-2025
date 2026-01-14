#include "graphics/util/renderpass-copy.hpp"

#include "asset/shader/copy-1.frag.hpp"
#include "asset/shader/copy-2.frag.hpp"
#include "asset/shader/copy-4.frag.hpp"
#include "gpu/graphics-pipeline.hpp"

#include <SDL3/SDL_gpu.h>
#include <array>
#include <format>

namespace graphics
{
	std::expected<RenderpassCopy, util::Error> RenderpassCopy::create(
		SDL_GPUDevice* device,
		size_t channels,
		gpu::Texture::Format dst_format
	) noexcept
	{
		std::span<const std::byte> fragment_code;

		switch (channels)
		{
		case 1:
			fragment_code = shader_asset::copy_1_frag;
			break;
		case 2:
			fragment_code = shader_asset::copy_2_frag;
			break;
		case 4:
			fragment_code = shader_asset::copy_4_frag;
			break;
		default:
			return util::Error(std::format("Unsupported channel count for Renderpass_copy: {}", channels));
		}

		auto fragment_shader = gpu::GraphicsShader::create(
			device,
			fragment_code,
			gpu::GraphicsShader::Stage::Fragment,
			1,
			0,
			0,
			0
		);
		if (!fragment_shader)
			return fragment_shader.error().forward("Create fragment shader for Renderpass_copy failed");

		auto copy_pass = FullscreenPass<true>::create(
			device,
			*fragment_shader,
			dst_format,
			{.clear_before_render = false, .blend_mode = FullscreenBlendMode::Overwrite},
			std::format("Renderpass_copy Pipeline ({} channels)", channels)
		);
		if (!copy_pass) return copy_pass.error().forward("Create FullscreenPass for Renderpass_copy failed");

		auto sampler = gpu::Sampler::create(
			device,
			gpu::Sampler::CreateInfo{
				.min_filter = gpu::Sampler::Filter::Nearest,
				.mag_filter = gpu::Sampler::Filter::Nearest,
				.address_mode_u = gpu::Sampler::AddressMode::Clamp_to_edge,
				.address_mode_v = gpu::Sampler::AddressMode::Clamp_to_edge,
				.address_mode_w = gpu::Sampler::AddressMode::Clamp_to_edge
			}
		);
		if (!sampler) return sampler.error().forward("Create sampler for Renderpass_copy failed");

		return RenderpassCopy(std::move(*copy_pass), std::move(*sampler));
	}

	std::expected<void, util::Error> RenderpassCopy::copy(
		const gpu::CommandBuffer& command_buffer,
		SDL_GPUTexture* src,
		SDL_GPUTexture* dst
	) const noexcept
	{
		command_buffer.push_debug_group("Copy texture via Renderpass_copy");
		const SDL_GPUTextureSamplerBinding sampler_binding{.texture = src, .sampler = sampler};
		auto result =
			copy_pass
				.render(command_buffer, dst, std::to_array({sampler_binding}), std::nullopt, std::nullopt);
		command_buffer.pop_debug_group();
		return result;
	}
}