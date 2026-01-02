#pragma once

#include "render/param.hpp"

namespace logic
{
	class Time_controller
	{
		struct Time_label
		{
			const char* icon;
			uint8_t hour;
			uint8_t minute;
		};

		static constexpr auto time_labels = std::to_array<Time_label>({
			{.icon = "\ue34c", .hour = 6,  .minute = 30}, // Sunrise, 6:30
			{.icon = "\uf522", .hour = 12, .minute = 0 }, // Noon, 12:00
			{.icon = "\ue34d", .hour = 17, .minute = 30}, // Sunset, 17:30
			{.icon = "\uf186", .hour = 0,  .minute = 0 }  // Midnight, 0:00
		});

		static constexpr auto time_wrap_options = std::to_array<double>({1.0, 10.0, 60.0, 600.0, 3600.0});

		bool time_flowing = false;
		uint32_t time_wrap_index = 0;
		double time_of_day = 8.0 * 3600.0;  // Time of day in seconds

		const float max_brightness = 80000.0;
		const float max_temperature = 5000.0;
		const float min_temperature = 1000.0;
		const float max_pitch = glm::radians(45.0f);
		const float sunrise_azimuth = glm::radians(-90.0f);
		const float day_ambient_intensity = 30.0f;

		void time_wrap_popup() noexcept;

	  public:

		double update() noexcept;

		void control_ui() noexcept;

		std::tuple<render::Primary_light_params, render::Ambient_params> get_sun_params() const noexcept;
	};
}