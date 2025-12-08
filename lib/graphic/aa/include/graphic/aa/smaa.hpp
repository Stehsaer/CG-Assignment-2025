#pragma once

#include "base.hpp"

#include <graphic/util/fullscreen-pass.hpp>
#include <graphic/util/smart-texture.hpp>

namespace graphic::aa
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
			const gpu::Command_buffer& command_buffer,
			SDL_GPUTexture* source,
			SDL_GPUTexture* target,
			glm::u32vec2 size
		) noexcept override;

	  private:

		SMAA(
			gpu::Sampler sampler,
			gpu::Texture blend_lut,
			Fullscreen_pass pass1,
			Fullscreen_pass pass2,
			Fullscreen_pass pass3
		) noexcept;

		gpu::Sampler sampler;
		gpu::Texture blend_lut;

		Fullscreen_pass pass1;
		Fullscreen_pass pass2;
		Fullscreen_pass pass3;

		Smart_texture edge_texture;
		Smart_texture blend_texture;
	};

}