#pragma once

#include "gltf/model.hpp"
#include "logic/camera-control.hpp"
#include "logic/light-source.hpp"
#include "render/drawdata/light.hpp"
#include "render/light-volume.hpp"
#include "logic/climate-viewer.hpp"
#include "logic/day-night-cycle.hpp"
#include "logic/section-view.hpp"
#include "render/param.hpp"

#include <glm/glm.hpp>
#include <glm/trigonometric.hpp>

class Logic
{
	/* Camera */

	logic::Camera camera_control;

	/* Lighting Control */

	float light_azimuth = glm::radians(159.0);
	float light_pitch = glm::radians(35.0);

	glm::vec3 light_color = {1.0, 1.0, 1.0};
	float light_intensity = 80000.0;
	float bloom_attenuation = 1.2f;
	float bloom_strength = 0.025f;
	bool use_bloom_mask = true;
	bool show_ceiling = true;
	float ambient_intensity = 50;

	uint32_t ceiling_node_index = 0;

	float csm_linear_blend = 0.56f;

	void light_control_ui() noexcept;

	/* Day-Night Cycle System */

	logic::Day_night_cycle day_night_cycle;

	/* Section View Control */

	logic::Section_view section_view;

	/* Climate Viewer */

	logic::Climate_viewer climate_viewer;

	void climate_control_ui() noexcept;

	/* Antialiasing */

	render::Antialias_mode aa_mode = render::Antialias_mode::MLAA;

	void antialias_control_ui() noexcept;

	/* Statistics */

	void statistic_display_ui() const noexcept;

	/* Animations */

	const float max_door_time = 53.0f / 24.0f;  // seconds
	const float max_door5_time = 5.0;
	const float max_curtain_time = 72.0f / 24.0f;  // seconds

	struct Animation_state
	{
		static constexpr float lerp_factor = 2;

		float current = 0.0f;
		float target = 0.0f;
		float max_time;
		std::string key;

		Animation_state(float max_time, std::string key) :
			max_time(max_time),
			key(std::move(key))
		{}

		void update(float dt) noexcept;
		float get_time() const noexcept;
	};

	struct Animation_metric
	{
		size_t animation_index;
		float distance;
		float dot;
		glm::vec2 puv;

		static Animation_metric from(
			size_t animation_index,
			const render::Camera_matrices& camera_matrices,
			glm::mat4 handle_matrix,
			glm::vec3 forward_dir
		) noexcept;
	};

	Animation_state door1_animation{max_door_time, "Door1"};
	Animation_state door2_animation{max_door_time, "Door2"};
	Animation_state door3_animation{max_door_time, "Door3"};
	Animation_state door4_animation{max_door_time, "Door4"};
	Animation_state door5_animation{max_door5_time, "Door5"};
	Animation_state curtain_left_animation{max_curtain_time, "CurtainLeft"};
	Animation_state curtain_right_animation{max_curtain_time, "CurtainRight"};

	uint32_t door1_node_index = 0;
	uint32_t door2_node_index = 0;
	uint32_t door3_node_index = 0;
	uint32_t door4_node_index = 0;
	uint32_t door5_node_index = 0;
	uint32_t curtain_left_node_index = 0;
	uint32_t curtain_right_node_index = 0;

	void animation_control_ui() noexcept;

	/* Lights */

	std::map<std::string, logic::Light_group> light_groups;
	void light_source_control_ui() noexcept;

	Logic() = default;

  public:

	Logic(const Logic&) = delete;
	Logic(Logic&&) = default;
	Logic& operator=(const Logic&) = delete;
	Logic& operator=(Logic&&) = delete;

	static std::expected<Logic, util::Error> create(SDL_GPUDevice* device, const gltf::Model& model) noexcept;

	std::tuple<render::Params, std::vector<gltf::Drawdata>, std::vector<render::drawdata::Light>> logic(
		const backend::SDL_context& context,
		const gltf::Model& model
	) noexcept;
};