#pragma once

#include "base.hpp"

#include <graphic/util/fullscreen-pass.hpp>

namespace graphic::aa
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
			const gpu::Command_buffer& command_buffer,
			SDL_GPUTexture* source,
			SDL_GPUTexture* target,
			glm::u32vec2 size
		) noexcept override;

	  private:

		FXAA(Fullscreen_pass fxaa_pass, gpu::Sampler sampler) noexcept;

		gpu::Sampler sampler;
		Fullscreen_pass fxaa_pass;
	};

}