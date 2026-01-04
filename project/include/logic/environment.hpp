#pragma once

#include "area.hpp"
#include "gltf/model.hpp"
#include "render/param.hpp"
#include "util/error.hpp"

#include <array>
#include <expected>
#include <glm/glm.hpp>
#include <optional>
#include <span>

namespace logic
{
	struct Climate
	{
		double temperature = 25;
		double humidity = 40;
		double pm2_5 = 12;
		double carbon_oxide = 0;
	};

	class Environment
	{
	  public:

		static std::expected<Environment, util::Error> create(const gltf::Model& model) noexcept;

		///
		/// @brief Control UI for climate data.
		///
		///
		void control_ui() noexcept;

		///
		/// @brief Chart UI for climate data.
		///
		///
		void chart_ui() noexcept;

		///
		/// @brief HUD UI for climate data
		///
		/// @param node_matrices Node transformation matrices
		/// @param camera_matrices Camera transformation matrices
		///
		void hud_ui(
			std::span<const glm::mat4> node_matrices,
			const render::Camera_matrices& camera_matrices
		) const noexcept;

		struct Update_result
		{
			std::optional<Area> fire_alert = std::nullopt;
			bool bad_outdoor_air = false;
		};

		///
		/// @brief Update the climate environment simulation.
		///
		/// @param sim_time Current simulation time, in seconds
		/// @return Alerts and events
		///
		Update_result update(double sim_time) noexcept;

	  private:

		static constexpr float sim_tau = 1.5f * 3600.0f;  // Time constant for simulation (1.5 hours)

		static constexpr std::pair temp_range = {5.0, 25.0};       // Temperature range in a day
		static constexpr std::pair humidity_range = {40.0, 90.0};  // Humidity range in a day
		static constexpr std::pair pm25_range = {30.0, 70.0};      // PM2.5 range in a day
		static constexpr std::pair co_range = {0.0, 10.0};         // CO range in a day

		static constexpr double max_temp_hour = 14.0;     // Hour of the day when temperature is highest
		static constexpr double max_humidity_hour = 6.0;  // Hour of the day when humidity is highest
		static constexpr double pm25_peak_hour = 8.0;     // Hour of the day when PM2.5 peaks
		static constexpr double co_peak_hour = 18.0;      // Hour of the day when CO peaks

		static constexpr double fire_pm25_threshold = 400.0;  // PM2.5 level indicating possible fire
		static constexpr double fire_co_threshold = 40.0;     // CO level indicating possible fire

		static constexpr double bad_air_pm25_threshold = 150.0;  // PM2.5 level indicating bad outdoor air

		// stores the links between areas for climate exchange
		// (from, to)
		static constexpr std::array links = std::to_array({
			std::pair{Area::Living_room, Area::Kitchen      },
			std::pair{Area::Living_room, Area::Large_bedroom},
			std::pair{Area::Living_room, Area::Small_bedroom},
			std::pair{Area::Living_room, Area::Toilet       },
			std::pair{Area::Exterior,    Area::Living_room  },
			std::pair{Area::Exterior,    Area::Kitchen      },
			std::pair{Area::Exterior,    Area::Toilet       },
			std::pair{Area::Exterior,    Area::Large_bedroom},
		});

		std::map<Area, Climate> area_climates = {
			{Area::Living_room,   {}},
			{Area::Large_bedroom, {}},
			{Area::Kitchen,       {}},
			{Area::Toilet,        {}},
			{Area::Small_bedroom, {}},
			{Area::Exterior,      {}},
		};

		const std::map<Area, uint32_t> area_node_indices;
		static const std::map<Area, std::string> area_node_names;

		std::optional<double> prev_sim_time = std::nullopt;

		void draw_bars() const noexcept;
		void draw_comparison_table() const noexcept;

		static Climate generate_outdoor_climate(double sim_time) noexcept;

		Environment(std::map<Area, uint32_t> area_node_indices) :
			area_node_indices(std::move(area_node_indices))
		{}

	  public:

		Environment(const Environment&) = default;
		Environment(Environment&&) = default;
		Environment& operator=(const Environment&) = delete;
		Environment& operator=(Environment&&) = delete;
	};
}