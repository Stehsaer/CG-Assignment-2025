#pragma once

#include "gpu/command-buffer.hpp"
#include "gpu/compute-pipeline.hpp"
#include "gpu/sampler.hpp"
#include "gpu/texture.hpp"
#include "target/gbuffer.hpp"
#include "target/light.hpp"
#include "target/ssgi.hpp"
#include "util/error.hpp"

#include <expected>
#include <glm/glm.hpp>

namespace pipeline
{
	class SSGI
	{
	  public:

		struct Param
		{
			glm::mat4 proj_mat;        // Camera projection matrix
			glm::mat4 view_mat;        // Camera view matrix
			float max_scene_distance;  // farthest viewable distance in the scene
			float distance_attenuation;  // attenuation factor for distance
		};

		static std::expected<SSGI, util::Error> create(SDL_GPUDevice* device) noexcept;

		std::expected<void, util::Error> render(
			const gpu::Command_buffer& command_buffer,
			const target::Light_buffer& light_buffer,
			const target::Gbuffer& gbuffer,
			const target::SSGI& ssgi_target,
			const Param& param,
			glm::u32vec2 resolution
		) noexcept;

	  private:

		struct Internal_param
		{
			glm::mat4 inv_proj_mat;
			glm::mat4 proj_mat;
			glm::mat4 view_mat;

			glm::vec4 inv_proj_mat_col3;
			glm::vec4 inv_proj_mat_col4;

			glm::uvec2 resolution;
			glm::ivec2 time_noise;

			glm::vec2 near_plane_span;
			float near_plane;
			float max_scene_distance;  // farthest viewable distance in the scene
			float distance_attenuation;  // attenuation factor for distance

			static Internal_param from_param(const Param& param, const glm::uvec2& resolution) noexcept;
		};

		gpu::Compute_pipeline ssgi_pipeline;
		gpu::Sampler noise_sampler, nearest_sampler;
		gpu::Texture noise_texture;

		SSGI(
			gpu::Compute_pipeline ssgi_pipeline,
			gpu::Sampler noise_sampler,
			gpu::Sampler nearest_sampler,
			gpu::Texture noise_texture
		) :
			ssgi_pipeline(std::move(ssgi_pipeline)),
			noise_sampler(std::move(noise_sampler)),
			nearest_sampler(std::move(nearest_sampler)),
			noise_texture(std::move(noise_texture))
		{}

	  public:

		SSGI(const SSGI&) = delete;
		SSGI(SSGI&&) = default;
		SSGI& operator=(const SSGI&) = delete;
		SSGI& operator=(SSGI&&) = default;
	};
}