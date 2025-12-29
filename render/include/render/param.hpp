#pragma once

#include <glm/glm.hpp>

namespace render
{
	enum class Antialias_mode
	{
		None,
		FXAA,
		MLAA,
		SMAA
	};

	struct Camera_matrices
	{
		glm::mat4 view_matrix;
		glm::mat4 proj_matrix;
		glm::mat4 prev_view_proj_matrix;
		glm::vec3 eye_position;
	};

	struct Primary_light_params
	{
		glm::vec3 direction;
		glm::vec3 intensity;
	};

	struct Ambient_params
	{
		glm::vec3 intensity = glm::vec3(50);
		float ao_radius = 70.0;
		float ao_blend_ratio = 0.017;
		float ao_strength = 1.0;
	};

	struct Bloom_params
	{
		float bloom_attenuation = 1.2;
		float bloom_strength = 0.025;
	};

	struct Shadow_params
	{
		float csm_linear_blend = 0.56;
	};

	struct Sky_params
	{
		float brightness;
		float turbidity = 2.0;
		float brightness_mult = 0.1;
	};

	struct Function_mask
	{
		bool ssgi = true;
		bool use_bloom_mask = true;
	};

	struct Params
	{
		Antialias_mode aa_mode = Antialias_mode::MLAA;
		Camera_matrices camera;
		Primary_light_params primary_light;
		Ambient_params ambient = {};
		Bloom_params bloom = {};
		Shadow_params shadow = {};
		Sky_params sky = {};
		Function_mask function_mask = {};
	};

	struct Light_drawcall
	{};
}