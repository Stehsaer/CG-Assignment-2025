#pragma once

#include "base.hpp"

#include <graphic/util/fullscreen-pass.hpp>
#include <graphic/util/smart-texture.hpp>

namespace graphic::aa
{
	///
	/// @brief MLAA Antialiasing Processor
	///
	///
	class MLAA : public Processor
	{
	  public:

		MLAA(const MLAA&) noexcept = delete;
		MLAA& operator=(const MLAA&) noexcept = delete;
		MLAA(MLAA&&) noexcept = default;
		MLAA& operator=(MLAA&&) noexcept = default;

		///
		/// @brief Create an MLAA processor
		///
		/// @param format Target texture format, see `target` parameter of @p run_antialiasing
		/// @return `MLAA` object on success, or `util::Error` on failure
		///
		static std::expected<MLAA, util::Error> create(
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

		MLAA(
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