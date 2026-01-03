#pragma once

#include "area.hpp"
#include "gltf/animation.hpp"
#include "gltf/model.hpp"
#include "render/param.hpp"
#include "util/error.hpp"

namespace logic
{
	class Furniture_controller
	{
	  public:

		static std::expected<Furniture_controller, util::Error> create(const gltf::Model& model) noexcept;

		void control_ui() noexcept;

		///
		/// @brief Display HUD UI for furniture items.
		///
		/// @param node_vertices Node transformation matrices from main drawdata.
		/// @param camera_matrices Current camera matrices
		///
		void hud_ui(
			std::span<const glm::mat4> node_vertices,
			const render::Camera_matrices& camera_matrices
		) noexcept;

		///
		/// @brief Update and get animation keys for furniture items.
		///
		/// @return Animation keys for furniture items.
		///
		std::vector<gltf::Animation_key> update() noexcept;

		///
		/// @brief Handle fire event by closing all furniture items in the specified area.
		///
		/// @param fire_area Area where the fire event occurred.
		///
		void handle_fire_event(Area fire_area) noexcept;

	  private:

		struct Config
		{
			float max_animation_time;
			bool hud;
			std::optional<Area> fire_area;
			std::string display_name;
			std::string animation_name;
			std::string node_name;
		};

		struct State
		{
			const Config config;
			const uint32_t node_index;

			bool opened = false;
			float current_time = 0.0f;

			void update(float delta_time) noexcept;
		};

		std::vector<State> furniture_states;
		static const std::vector<Config> furniture_configs;

		static constexpr float max_hud_distance = 3.0f;

		Furniture_controller(std::vector<State> furniture_states) :
			furniture_states(std::move(furniture_states))
		{}

	  public:

		Furniture_controller(const Furniture_controller&) = delete;
		Furniture_controller(Furniture_controller&&) = default;
		Furniture_controller& operator=(const Furniture_controller&) = delete;
		Furniture_controller& operator=(Furniture_controller&&) = default;
	};
}