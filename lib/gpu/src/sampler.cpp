#include "gpu/sampler.hpp"
#include "gpu/util.hpp"

namespace gpu
{
	SDL_GPUSamplerCreateInfo Sampler::CreateInfo::create() const noexcept
	{
		const SDL_GPUSamplerCreateInfo info{
			.min_filter = static_cast<SDL_GPUFilter>(min_filter),
			.mag_filter = static_cast<SDL_GPUFilter>(mag_filter),
			.mipmap_mode = static_cast<SDL_GPUSamplerMipmapMode>(mipmap_mode),
			.address_mode_u = static_cast<SDL_GPUSamplerAddressMode>(address_mode_u),
			.address_mode_v = static_cast<SDL_GPUSamplerAddressMode>(address_mode_v),
			.address_mode_w = static_cast<SDL_GPUSamplerAddressMode>(address_mode_w),
			.mip_lod_bias = mip_lod_bias,
			.max_anisotropy = max_anisotropy.value_or(1.0f),
			.compare_op = static_cast<SDL_GPUCompareOp>(compare_op.value_or(CompareOp::Always)),
			.min_lod = min_lod,
			.max_lod = max_lod,
			.enable_anisotropy = max_anisotropy.has_value(),
			.enable_compare = compare_op.has_value(),
			.padding1 = 0,
			.padding2 = 0,
			.props = 0
		};

		return info;
	}

	std::expected<Sampler, util::Error> Sampler::create(
		SDL_GPUDevice* device,
		const CreateInfo& create_info
	) noexcept
	{
		assert(device != nullptr);

		const auto sdl_create_info = create_info.create();

		auto* const sampler = SDL_CreateGPUSampler(device, &sdl_create_info);
		if (sampler == nullptr) RETURN_SDL_ERROR;
		return Sampler(device, sampler);
	}
}