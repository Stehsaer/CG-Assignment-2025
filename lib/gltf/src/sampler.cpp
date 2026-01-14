#include "gltf/sampler.hpp"

namespace gltf
{
	static gpu::Sampler::Filter get_filter_mode(int mode) noexcept
	{
		switch (mode)
		{
		case TINYGLTF_TEXTURE_FILTER_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
			return gpu::Sampler::Filter::Nearest;

		case TINYGLTF_TEXTURE_FILTER_LINEAR:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
		default:
			return gpu::Sampler::Filter::Linear;
		}
	}

	static gpu::Sampler::MipmapMode get_mipmap_mode(int mode) noexcept
	{
		switch (mode)
		{
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
			return gpu::Sampler::MipmapMode::Nearest;

		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
		default:
			return gpu::Sampler::MipmapMode::Linear;
		}
	}

	static gpu::Sampler::AddressMode get_address_mode(int mode) noexcept
	{
		switch (mode)
		{
		case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
			return gpu::Sampler::AddressMode::Clamp_to_edge;

		case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
			return gpu::Sampler::AddressMode::Mirrored_repeat;

		case TINYGLTF_TEXTURE_WRAP_REPEAT:
		default:
			return gpu::Sampler::AddressMode::Repeat;
		}
	}

	std::expected<gpu::Sampler, util::Error> create_sampler(
		SDL_GPUDevice* device,
		const tinygltf::Sampler& sampler,
		const SamplerConfig& config
	) noexcept
	{
		const gpu::Sampler::CreateInfo create_info{
			.min_filter = get_filter_mode(sampler.minFilter),
			.mag_filter = get_filter_mode(sampler.magFilter),
			.mipmap_mode = get_mipmap_mode(sampler.minFilter),
			.address_mode_u = get_address_mode(sampler.wrapS),
			.address_mode_v = get_address_mode(sampler.wrapT),
			.mip_lod_bias = config.lod_bias,
			.max_anisotropy = config.anisotropy
		};

		return gpu::Sampler::create(device, create_info);
	}
}