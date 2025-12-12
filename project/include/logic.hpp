#pragma once

#include "gltf/model.hpp"
#include "logic/ambient-light.hpp"
#include "logic/camera-control.hpp"
#include "renderer/aa.hpp"

#include <glm/glm.hpp>
#include <glm/trigonometric.hpp>

struct Draw_feedback
{
	size_t total_drawcall_count;
	size_t visible_drawcall_count;
	size_t triangle_count;
};

class Logic
{
  public:

	enum class Debug_mode
	{
		No_debug,
		Show_AO,
		Show_SSGI_trace
	};

	struct Result
	{
		renderer::Antialias::Mode aa_mode;
		logic::Ambient_light ambient_lighting;
		logic::Camera::Matrices camera;

		glm::vec3 light_direction;
		glm::vec3 light_color;
		float turbidity;
		float sky_brightness_mult;
		float bloom_attenuation;
		float bloom_strength;

		Debug_mode debug_mode;

		float csm_linear_blend;

		std::vector<gltf::Animation_key> animation_keys;
	};

  private:

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
	float bloom_strength = 0.06f;

	float csm_linear_blend = 0.56f;

	logic::Ambient_light ambient_lighting;

	void light_control_ui() noexcept;

	/* Antialiasing */

	renderer::Antialias::Mode aa_mode = renderer::Antialias::Mode::MLAA;

	void antialias_control_ui() noexcept;

	/* Statistics */

	void statistic_display_ui() const noexcept;

	/* Debug */

	Debug_mode debug_mode = Debug_mode::No_debug;
	float time = 0.0f;
	bool time_run = false;

	void debug_control_ui() noexcept;

  public:

	Result logic(const gltf::Model& model) noexcept;
};