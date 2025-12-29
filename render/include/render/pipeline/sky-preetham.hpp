#pragma once

#include "gpu/sampler.hpp"
#include "graphics/util/fullscreen-pass.hpp"

#include <glm/glm.hpp>

namespace render::pipeline
{
	///
	/// @brief Preetham sky model
	/// @note Algorithm adapted from https://github.com/diharaw/SkyModels
	///
	class Sky_preetham
	{
	  public:

		struct Params
		{
			glm::mat4 camera_mat_inv;
			glm::uvec2 screen_size;
			glm::vec3 eye_position;
			glm::vec3 sun_direction;
			glm::vec3 sun_intensity;
			float turbidity;
		};

		static std::expected<Sky_preetham, util::Error> create(SDL_GPUDevice* device) noexcept;

		void render(
			const gpu::Command_buffer& command_buffer,
			const gpu::Render_pass& render_pass,
			const Params& params
		) const noexcept;

	  private:

		struct alignas(16) Preetham_params
		{
			alignas(16) glm::vec3 A;
			alignas(16) glm::vec3 B;
			alignas(16) glm::vec3 C;
			alignas(16) glm::vec3 D;
			alignas(16) glm::vec3 E;
			alignas(16) glm::vec3 Z;

			Preetham_params(float turbidity, glm::vec3 sun_direction) noexcept;
		};

		struct Internal_params
		{
			glm::mat4 camera_mat_inv;
			glm::vec2 screen_size;
			alignas(16) glm::vec3 eye_position;
			alignas(16) glm::vec3 sun_direction;
			alignas(16) glm::vec3 sun_intensity;
			float intensity;

			Preetham_params preetham;

			Internal_params(const Params& params) noexcept;
		};

		gpu::Sampler sampler;
		graphics::Fullscreen_pass<false> fullscreen_pass;

		Sky_preetham(gpu::Sampler sampler, graphics::Fullscreen_pass<false> fullscreen_pass) noexcept :
			sampler(std::move(sampler)),
			fullscreen_pass(std::move(fullscreen_pass))
		{}

	  public:

		Sky_preetham(const Sky_preetham&) = delete;
		Sky_preetham(Sky_preetham&&) = default;
		Sky_preetham& operator=(const Sky_preetham&) = delete;
		Sky_preetham& operator=(Sky_preetham&&) = default;
	};
}