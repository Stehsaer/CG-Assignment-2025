///
/// @file smaa.hpp
/// @brief Defines a SMAA antialiasing processor
///

#pragma once

#include "base.hpp"

#include "gpu/sampler.hpp"
#include "gpu/texture.hpp"
#include "graphics/util/fullscreen-pass.hpp"
#include "graphics/util/smart-texture.hpp"

namespace graphics::aa
{
	///
	/// @brief SMAA Antialiasing Processor
	///
	///
	class SMAA : public Processor
	{
	  public:

		SMAA(const SMAA&) noexcept = delete;
		SMAA& operator=(const SMAA&) noexcept = delete;
		SMAA(SMAA&&) noexcept = default;
		SMAA& operator=(SMAA&&) noexcept = default;

		///
		/// @brief Create an SMAA processor
		///
		/// @param format Target texture format, see `target` parameter of @p run_antialiasing
		/// @return `SMAA` object on success, or `util::Error` on failure
		///
		static std::expected<SMAA, util::Error> create(
			SDL_GPUDevice* device,
			SDL_GPUTextureFormat format
		) noexcept;

		std::expected<void, util ::Error> run_antialiasing(
			SDL_GPUDevice* device,
			const gpu::CommandBuffer& command_buffer,
			SDL_GPUTexture* source,
			SDL_GPUTexture* target,
			glm::u32vec2 size
		) noexcept override;

	  private:

		SMAA(
			gpu::Sampler sampler,
			gpu::Texture blend_lut,
			gpu::Texture diag_lut,
			FullscreenPass<true> pass1,
			FullscreenPass<true> pass2,
			FullscreenPass<true> pass3
		) noexcept;

		gpu::Sampler sampler;
		gpu::Texture blend_lut;
		gpu::Texture diag_lut;

		FullscreenPass<true> pass1;
		FullscreenPass<true> pass2;
		FullscreenPass<true> pass3;

		AutoTexture edge_texture;
		AutoTexture blend_texture;
	};

}