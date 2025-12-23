#pragma once

#include "gltf/model.hpp"
#include "logic/ambient-light.hpp"
#include "logic/camera-control.hpp"
#include "render/param.hpp"

#include <glm/glm.hpp>
#include <glm/trigonometric.hpp>

class Logic
{
	/* Camera */

	logic::Camera camera_control;

	/* Lighting Control */

	float light_azimuth = glm::radians(103.0);
	float light_pitch = glm::radians(53.0);

	glm::vec3 light_color = {1.0, 1.0, 1.0};
	float light_intensity = 20.0;
	float turbidity = 2.0f;
	float sky_brightness_mult = 0.1f;
	float bloom_attenuation = 1.2f;
	float bloom_strength = 0.05f;
	bool use_bloom_mask = true;

	float csm_linear_blend = 0.56f;

	logic::Ambient_light ambient_lighting;

	void light_control_ui() noexcept;

	/* Antialiasing */

	render::Antialias_mode aa_mode = render::Antialias_mode::MLAA;

	void antialias_control_ui() noexcept;

	/* Statistics */

	void statistic_display_ui() const noexcept;

	/* Debug */

	float time = 0.0f;
	bool time_run = false;

	void debug_control_ui() noexcept;

  public:

	std::tuple<render::Params, std::vector<gltf::Drawdata>> logic(
		const backend::SDL_context& context,
		const gltf::Model& model
	) noexcept;
};