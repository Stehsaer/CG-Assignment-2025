#include "logic/time-controller.hpp"
#include "ui/capsule.hpp"

#include <format>
#include <glm/gtc/constants.hpp>
#include <imgui.h>
#include <ranges>
#include <utility>

namespace logic
{
	static glm::vec3 temperature_to_linear_color(float temperature_kelvin)
	{
		const float temp = glm::clamp(temperature_kelvin, 1000.0f, 40000.0f) / 100.0f;

		float red = 0.0f;
		float green = 0.0f;
		float blue = 0.0f;

		if (temp <= 66.0f)
		{
			red = 255.0f;
		}
		else
		{
			red = 329.698727446f * std::pow(temp - 60.0f, -0.1332047592f);
		}

		if (temp <= 66.0f)
		{
			green = 99.4708025861f * std::log(temp) - 161.1195681661f;
		}
		else
		{
			green = 288.1221695283f * std::pow(temp - 60.0f, -0.0755148492f);
		}

		if (temp >= 66.0f)
		{
			blue = 255.0f;
		}
		else if (temp <= 19.0f)
		{
			blue = 0.0f;
		}
		else
		{
			blue = 138.5177312231f * std::log(temp - 10.0f) - 305.0447927307f;
		}

		const auto to_srgb01 = [](float v) {
			return glm::clamp(v, 0.0f, 255.0f) / 255.0f;
		};

		const auto srgb = glm::vec3(to_srgb01(red), to_srgb01(green), to_srgb01(blue));

		// Convert sRGB (gamma encoded) to linear-light RGB using the inverse sRGB transfer function
		const auto srgb_to_linear = [](float c) {
			if (c <= 0.04045f) return c / 12.92f;
			return std::pow((c + 0.055f) / 1.055f, 2.4f);
		};

		return {srgb_to_linear(srgb.r), srgb_to_linear(srgb.g), srgb_to_linear(srgb.b)};
	}

	static glm::vec3 spherical_coord(float pitch, float azimuth)
	{
		return {glm::cos(pitch) * glm::sin(azimuth), glm::sin(pitch), glm::cos(pitch) * glm::cos(azimuth)};
	}

	double Time_controller::update() noexcept
	{
		const double dt = ImGui::GetIO().DeltaTime;

		if (time_flowing)
		{
			time_of_day += dt * time_wrap_options[time_wrap_index];
			time_of_day = std::fmod(time_of_day, 86400.0);
		}

		return time_of_day;
	}

	void Time_controller::settings_panel() noexcept
	{
		if (settings_panel_opened)
			ui::capsule::window(
				"时间设置",
				ui::capsule::Position::Bottom_center,
				[this] {
					ImGui::AlignTextToFramePadding();

					if (ui::capsule::button("\uf049")) time_of_day -= 3600.0;
					if (ui::capsule::button("\uf048")) time_of_day -= 60.0;

					ui::capsule::label(
						std::format(
							"{:02}:{:02}:{:02}",
							static_cast<int>(time_of_day) / 3600,
							(static_cast<int>(time_of_day) % 3600) / 60,
							static_cast<int>(time_of_day) % 60
						)
					);

					if (ui::capsule::button("\uf051")) time_of_day += 60.0;
					if (ui::capsule::button("\uf050")) time_of_day += 3600.0;
					time_of_day = std::fmod(time_of_day + 86400.0, 86400.0);

					ImGui::NewLine();
					ImGui::Separator();

					if (ui::capsule::button("\uf068"))
						if (time_wrap_index > 0) time_wrap_index--;
					if (ui::capsule::button("\uf067"))
						if (time_wrap_index + 1 < time_wrap_options.size()) time_wrap_index++;

					ImGui::AlignTextToFramePadding();
					ui::capsule::label(std::format("{:.0f}x", time_wrap_options[time_wrap_index]));
				},
				{0, -1},
				true
			);
	}

	void Time_controller::control_ui() noexcept
	{
		if (!ImGui::GetIO().WantCaptureKeyboard && ImGui::IsKeyPressed(ImGuiKey_Space))
			time_flowing = !time_flowing;

		ui::capsule::window("##TimeControl", ui::capsule::Position::Bottom_center, [this] {
			if (settings_panel_opened)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
				ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 255, 255, 200));
				if (ui::capsule::button("\uf013")) settings_panel_opened = !settings_panel_opened;
				ImGui::PopStyleColor();
				ImGui::PopStyleVar();
			}
			else
			{
				if (ui::capsule::button("\uf013")) settings_panel_opened = !settings_panel_opened;
			}

			if (ui::capsule::button(std::format("{}##TimeFlowSwitch", time_flowing ? "\uf04c" : "\uf04b")))
				time_flowing = !time_flowing;

			ui::capsule::label(
				std::format(
					"{:02}:{:02}:{:02}",
					static_cast<int>(time_of_day) / 3600,
					(static_cast<int>(time_of_day) % 3600) / 60,
					static_cast<int>(time_of_day) % 60
				)
			);

			ui::capsule::vertical_separator();

			for (const auto [icon, hour, minute] : time_labels)
			{
				if (ui::capsule::button(std::format("{}##TimeJump{}{}", icon, hour, minute)))
					time_of_day = hour * 3600.0 + minute * 60.0;
			}
		});

		settings_panel();
	}

	std::tuple<render::Primary_light_params, render::Ambient_params>
	Time_controller::get_sun_params() const noexcept
	{
		const auto time_in_hours = static_cast<float>(time_of_day / 3600.0);

		const auto to_sunrise_dir = spherical_coord(0, sunrise_azimuth);
		const auto to_noon_dir = spherical_coord(max_pitch, sunrise_azimuth - glm::radians(90.0f));

		const auto angle = (time_in_hours - 6.0f) / 12.0f * std::numbers::pi_v<float>;
		const auto sun_dir = glm::normalize(glm::cos(angle) * to_sunrise_dir + glm::sin(angle) * to_noon_dir);
		const auto sun_pitch = glm::asin(sun_dir.y);

		const auto sun_horizonal_brightness_mult =
			glm::smoothstep(glm::radians(0.0f), glm::radians(15.0f), sun_pitch);
		const auto temperature = glm::mix(1000.0f, 5000.0f, sun_horizonal_brightness_mult);
		const auto sun_color = temperature_to_linear_color(temperature);
		const float ambient_intensity =
			glm::clamp(std::sin((time_in_hours - 6.0f) / 24.0f * glm::two_pi<float>()), 0.0f, 1.0f);

		const render::Primary_light_params primary_light{
			.direction = sun_dir,
			.intensity = sun_color * (sun_horizonal_brightness_mult * max_brightness),
		};

		const render::Ambient_params ambient{
			.intensity = glm::vec3(ambient_intensity * day_ambient_intensity),
		};

		return {primary_light, ambient};
	}
}