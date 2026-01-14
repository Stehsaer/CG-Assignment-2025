///
/// @file fxaa.hpp
/// @brief Defines a FXAA antialiasing processor
///

#pragma once

#include "base.hpp"

#include "gpu/sampler.hpp"
#include "graphics/util/fullscreen-pass.hpp"

namespace graphics::aa
{
	///
	/// @brief FXAA Antialiasing Processor
	///
	class FXAA : public Processor
	{
	  public:

		FXAA(const FXAA&) noexcept = delete;
		FXAA& operator=(const FXAA&) noexcept = delete;
		FXAA(FXAA&&) noexcept = default;
		FXAA& operator=(FXAA&&) noexcept = default;

		~FXAA() noexcept override = default;

		///
		/// @brief Create an FXAA processor
		///
		/// @param format Target texture format, see `target` parameter of @p run_antialiasing
		/// @return `FXAA` object on success, or `util::Error` on failure
		///
		static std::expected<FXAA, util::Error> create(
			SDL_GPUDevice* device,
			SDL_GPUTextureFormat format
		) noexcept;

		std::expected<void, util::Error> run_antialiasing(
			SDL_GPUDevice* device,
			const gpu::CommandBuffer& command_buffer,
			SDL_GPUTexture* source,
			SDL_GPUTexture* target,
			glm::u32vec2 size
		) noexcept override;

	  private:

		FXAA(FullscreenPass<true> fxaa_pass, gpu::Sampler sampler) noexcept;

		gpu::Sampler sampler;
		FullscreenPass<true> fxaa_pass;
	};

}