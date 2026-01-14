#pragma once

#include <SDL3/SDL_gpu.h>
#include <expected>

#include "resource-box.hpp"
#include "util/error.hpp"

namespace gpu
{
	///
	/// @brief GPU Texture
	///
	///
	class Texture : public ResourceBox<SDL_GPUTexture>
	{
	  public:

		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;
		Texture(Texture&&) = default;
		Texture& operator=(Texture&&) = default;
		~Texture() noexcept = default;

		///
		/// @brief Create a GPU Texture
		///
		/// @param create_info Create Info
		/// @return Texture object, or error if failed
		///
		static std::expected<Texture, util::Error> create(
			SDL_GPUDevice* device,
			const SDL_GPUTextureCreateInfo& create_info,
			const std::string& name
		) noexcept;

		struct Usage
		{
			bool sampler                                 : 1 = false;
			bool color_target                            : 1 = false;
			bool depth_stencil_target                    : 1 = false;
			bool graphic_storage_read                    : 1 = false;
			bool compute_storage_read                    : 1 = false;
			bool compute_storage_write                   : 1 = false;
			bool compute_storage_simultaneous_read_write : 1 = false;

			operator SDL_GPUTextureUsageFlags(this Usage self) noexcept;
		};

		///
		/// @brief GPU Texture Format Description
		///
		///
		struct Format
		{
			SDL_GPUTextureType type;
			SDL_GPUTextureFormat format;
			Usage usage;

			///
			/// @brief Instantiate a texture creation info structure
			///
			/// @param width Width of the texture
			/// @param height Height of the texture
			/// @param depth Depth or layer count of the texture (default is 1)
			/// @param mip_levels Number of mip levels (default is 1)
			/// @param sample_count Sample Count (default is 1）
			/// @return Texture creation info structure
			///
			SDL_GPUTextureCreateInfo create(
				uint32_t width,
				uint32_t height,
				uint32_t depth = 1,
				uint32_t mip_levels = 1,
				SDL_GPUSampleCount sample_count = SDL_GPUSampleCount::SDL_GPU_SAMPLECOUNT_1
			) const noexcept;

			///
			/// @brief Query if the format is supported on a given device
			///
			/// @return `true` if supported, `false` otherwise
			///
			bool supported_on(SDL_GPUDevice* device) const noexcept;
		};

		///
		/// @brief Bind texture with sampler, and returns binding structure
		///
		/// @param sampler Sampler to bind with
		/// @return Binding structure
		///
		SDL_GPUTextureSamplerBinding bind_with_sampler(SDL_GPUSampler* sampler) const noexcept;

	  private:

		using ResourceBox<SDL_GPUTexture>::ResourceBox;
	};

}