#pragma once

#include "gpu/command-buffer.hpp"
#include "gpu/compute-pipeline.hpp"
#include "gpu/sampler.hpp"
#include "gpu/texture.hpp"
#include "graphics/util/fullscreen-pass.hpp"
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
			glm::mat4 prev_view_proj_mat;  // Previous frame view-projection matrix
			float max_scene_distance;  // farthest viewable distance in the scene
			float distance_attenuation;  // attenuation factor for distance
			float blend_factor;          // blending factor for temporal accumulation
		};

		static std::expected<SSGI, util::Error> create(SDL_GPUDevice* device) noexcept;

		std::expected<void, util::Error> render(
			const gpu::Command_buffer& command_buffer,
			const target::Light_buffer& light_buffer,
			const target::Gbuffer& gbuffer,
			const target::SSGI& ssgi_target,
			const Param& param,
			glm::u32vec2 resolution
		) const noexcept;

	  private:

		struct Initial_temporal_param
		{
			glm::mat4 inv_proj_mat;
			glm::mat4 proj_mat;
			glm::mat4 inv_view_mat;
			glm::mat4 view_mat;
			glm::mat4 back_proj_mat;

			glm::vec4 inv_proj_mat_col3;
			glm::vec4 inv_proj_mat_col4;

			glm::uvec2 resolution;
			glm::ivec2 time_noise;

			glm::vec2 near_plane_span;
			float near_plane;
			float max_scene_distance;  // farthest viewable distance in the scene
			float distance_attenuation;  // attenuation factor for distance

			static Initial_temporal_param from_param(
				const Param& param,
				const glm::uvec2& resolution
			) noexcept;
		};

		struct Spatial_reuse_param
		{
			glm::mat4 inv_view_proj_mat;
			glm::mat4 prev_view_proj_mat;
			glm::mat4 proj_mat;
			glm::mat4 view_mat;
			glm::mat4 inv_view_mat;
			glm::vec4 inv_proj_mat_col3;
			glm::vec4 inv_proj_mat_col4;

			glm::uvec2 comp_resolution;
			glm::uvec2 full_resolution;
			glm::vec2 near_plane_span;
			float near_plane;
			glm::ivec2 time_noise;

			static Spatial_reuse_param from_param(const Param& param, const glm::uvec2& resolution) noexcept;
		};

		struct Radiance_composite_param
		{
			glm::mat4 back_projection_mat;
			glm::mat4 inv_back_projection_mat;
			glm::mat4 inv_view_proj_mat;
			glm::mat4 inv_view_mat;
			glm::uvec2 comp_resolution;
			glm::uvec2 full_resolution;
			float blend_factor;

			static Radiance_composite_param from_param(const Param& param, glm::u32vec2 resolution) noexcept;
		};

		struct Radiance_upsample_param
		{
			glm::vec4 inv_view_proj_mat_col3;
			glm::vec4 inv_view_proj_mat_col4;
			glm::uvec2 comp_resolution;
			glm::uvec2 full_resolution;

			static Radiance_upsample_param from_param(const Param& param, glm::u32vec2 resolution) noexcept;
		};

		gpu::Compute_pipeline initial_pipeline;
		gpu::Compute_pipeline spatial_reuse_pipeline;
		gpu::Compute_pipeline radiance_composite_pipeline;
		gpu::Compute_pipeline radiance_upsample_pipeline;
		graphics::Fullscreen_pass<true> radiance_add_pass;
		gpu::Sampler noise_sampler, nearest_sampler, linear_sampler;
		gpu::Texture noise_texture;

		std::expected<void, util::Error> run_initial_sample(
			const gpu::Command_buffer& command_buffer,
			const target::Light_buffer& light_buffer,
			const target::Gbuffer& gbuffer,
			const target::SSGI& ssgi_target,
			const Param& param,
			glm::u32vec2 resolution
		) const noexcept;

		std::expected<void, util::Error> run_spatial_reuse(
			const gpu::Command_buffer& command_buffer,
			const target::Gbuffer& gbuffer,
			const target::SSGI& ssgi_target,
			const Param& param,
			glm::u32vec2 resolution
		) const noexcept;

		std::expected<void, util::Error> run_radiance_composite(
			const gpu::Command_buffer& command_buffer,
			const target::Gbuffer& gbuffer,
			const target::SSGI& ssgi_target,
			const Param& param,
			glm::u32vec2 resolution
		) const noexcept;

		std::expected<void, util::Error> run_radiance_upsample(
			const gpu::Command_buffer& command_buffer,
			const target::Gbuffer& gbuffer,
			const target::SSGI& ssgi_target,
			const Param& param,
			glm::u32vec2 resolution
		) const noexcept;

		std::expected<void, util::Error> render_radiance_add(
			const gpu::Command_buffer& command_buffer,
			const target::Light_buffer& light_buffer,
			const target::SSGI& ssgi_target,
			glm::u32vec2 resolution
		) const noexcept;

		SSGI(
			gpu::Compute_pipeline ssgi_pipeline,
			gpu::Compute_pipeline spatial_reuse_pipeline,
			gpu::Compute_pipeline radiance_composite_pipeline,
			gpu::Compute_pipeline radiance_upsample_pipeline,
			graphics::Fullscreen_pass<true> radiance_add_pass,
			gpu::Sampler noise_sampler,
			gpu::Sampler nearest_sampler,
			gpu::Sampler linear_sampler,
			gpu::Texture noise_texture
		) :
			initial_pipeline(std::move(ssgi_pipeline)),
			spatial_reuse_pipeline(std::move(spatial_reuse_pipeline)),
			radiance_composite_pipeline(std::move(radiance_composite_pipeline)),
			radiance_upsample_pipeline(std::move(radiance_upsample_pipeline)),
			radiance_add_pass(std::move(radiance_add_pass)),
			noise_sampler(std::move(noise_sampler)),
			nearest_sampler(std::move(nearest_sampler)),
			linear_sampler(std::move(linear_sampler)),
			noise_texture(std::move(noise_texture))
		{}

	  public:

		SSGI(const SSGI&) = delete;
		SSGI(SSGI&&) = default;
		SSGI& operator=(const SSGI&) = delete;
		SSGI& operator=(SSGI&&) = default;
	};
}