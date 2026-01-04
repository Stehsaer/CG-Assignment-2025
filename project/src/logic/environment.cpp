#include "logic/environment.hpp"
#include "ui/capsule.hpp"

#include <imgui-knobs.h>
#include <imgui.h>
#include <implot.h>

#include <algorithm>
#include <random>
#include <ranges>

namespace logic
{
	namespace color_palette
	{
		static ImVec4 get_temperature_color(float temperature) noexcept
		{
			float normalized = std::clamp((temperature - 15.0f) / 20.0f, 0.0f, 1.0f);

			if (normalized < 0.33f)
			{
				float t = normalized / 0.33f;
				return ImVec4(0.0f, 0.3f + t * 0.5f, 0.8f + t * 0.2f, 1.0f);
			}
			else if (normalized < 0.67f)
			{
				float t = (normalized - 0.33f) / 0.34f;
				return ImVec4(t * 0.5f, 0.8f, 1.0f - t * 0.5f, 1.0f);
			}
			else
			{
				float t = (normalized - 0.67f) / 0.33f;
				return ImVec4(0.5f + t * 0.5f, 0.8f - t * 0.7f, 0.5f - t * 0.5f, 1.0f);
			}
		}

		static ImVec4 get_humidity_color(float humidity) noexcept
		{
			float normalized = std::clamp(humidity / 100.0f, 0.0f, 1.0f);
			return ImVec4(1.0f - normalized * 0.7f, 0.7f - normalized * 0.2f, 0.3f + normalized * 0.7f, 1.0f);
		}

		static ImVec4 get_pm25_color(float pm25) noexcept
		{
			if (pm25 <= 35.0f)
			{
				float t = pm25 / 35.0f;
				return ImVec4(0.2f + t * 0.6f, 0.9f - t * 0.2f, 0.2f, 1.0f);
			}
			else if (pm25 <= 75.0f)
			{
				float t = (pm25 - 35.0f) / 40.0f;
				return ImVec4(0.8f + t * 0.2f, 0.7f - t * 0.2f, 0.2f - t * 0.2f, 1.0f);
			}
			else if (pm25 <= 150.0f)
			{
				float t = (pm25 - 75.0f) / 75.0f;
				return ImVec4(1.0f - t * 0.2f, 0.5f - t * 0.5f, 0.0f, 1.0f);
			}
			else
			{
				float t = std::min((pm25 - 150.0f) / 100.0f, 1.0f);
				return ImVec4(0.8f - t * 0.2f, 0.0f, 0.0f + t * 0.6f, 1.0f);
			}
		}

		static ImVec4 get_co_color(float co) noexcept
		{
			float normalized = std::clamp(co / 50.0f, 0.0f, 1.0f);
			if (normalized < 0.5f)
			{
				float t = normalized * 2.0f;
				return ImVec4(0.3f + t * 0.7f, 0.9f, 0.3f - t * 0.3f, 1.0f);
			}
			else
			{
				float t = (normalized - 0.5f) * 2.0f;
				return ImVec4(1.0f, 0.9f - t * 0.9f, 0.0f, 1.0f);
			}
		}
	}

	static void draw_knobs(Climate& climate) noexcept
	{
		float modified_temperature = climate.temperature;
		float modified_humidity = climate.humidity;
		float modified_pm2_5 = climate.pm2_5;
		float modified_carbon_oxide = climate.carbon_oxide;

		const auto scale = ImGui::GetStyle().FontScaleDpi;

		ImGuiKnobs::Knob(
			"温度",
			&modified_temperature,
			-30.0f,
			50.0f,
			1.0f,
			"%.1f °C",
			ImGuiKnobVariant_WiperDot,
			80.0f * scale,
			0
		);
		ImGui::SameLine();

		ImGuiKnobs::Knob(
			"湿度",
			&modified_humidity,
			0.0f,
			100.0f,
			1.0f,
			"%.1f %%RH",
			ImGuiKnobVariant_WiperDot,
			80.0f * scale,
			0
		);
		ImGui::SameLine();

		ImGuiKnobs::Knob(
			"PM2.5",
			&modified_pm2_5,
			0.0f,
			500.0f,
			1.0f,
			"%.1f µg/m³",
			ImGuiKnobVariant_WiperDot,
			80.0f * scale,
			0
		);
		ImGui::SameLine();

		ImGuiKnobs::Knob(
			"CO",
			&modified_carbon_oxide,
			0.0f,
			50.0f,
			0.1f,
			"%.2f ppm",
			ImGuiKnobVariant_WiperDot,
			80.0f * scale,
			0
		);

		climate.temperature = modified_temperature;
		climate.humidity = modified_humidity;
		climate.pm2_5 = modified_pm2_5;
		climate.carbon_oxide = modified_carbon_oxide;
	}

	static void draw_bar(const Climate& climate) noexcept
	{
		const auto scale = ImGui::GetStyle().FontScaleDpi;

		auto draw_component =
			[scale](
				const char* label,
				float value,
				float min_value,
				float max_value,
				auto color_func,
				auto format
			) {
				const float normalized =
					glm::clamp((value - min_value) / (max_value - min_value), 0.0f, 1.0f);
				const ImVec4 color = color_func(value);

				ImGui::TextUnformatted(label);
				ImGui::SameLine();
				ImGui::Indent(80.0f * scale);

				ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
				ImGui::ProgressBar(normalized, ImVec2(200.0f * scale, 20.0f * scale), "");
				ImGui::PopStyleColor();
				ImGui::SameLine();

				ImGui::Text(format, value);
				ImGui::Unindent(80.0f * scale);
			};

		draw_component(
			"温度",
			climate.temperature,
			0.0f,
			35.0f,
			color_palette::get_temperature_color,
			"%.1f °C"
		);
		draw_component(
			"湿度",
			climate.humidity,
			0.0f,
			100.0f,
			color_palette::get_humidity_color,
			"%.1f %%RH"
		);
		draw_component("PM2.5", climate.pm2_5, 0.0f, 200.0f, color_palette::get_pm25_color, "%.1f µg/m³");
		draw_component("CO", climate.carbon_oxide, 0.0f, 50.0f, color_palette::get_co_color, "%.2f ppm");
	}

	void Environment::control_ui() noexcept
	{
		ui::capsule::window(
			"##ClimateControl",
			ui::capsule::Position::Bottom_left,
			[this] {
				ImGui::BeginTabBar("ClimateTabs");
				for (auto& [area, climate] : area_climates
						 | std::views::filter([](auto pair) { return pair.first != Area::Exterior; }))
				{
					const auto name = area_names.at(area);
					if (ImGui::BeginTabItem(name))
					{
						draw_knobs(climate);
						ImGui::EndTabItem();
					}
				}
				ImGui::EndTabBar();
			},
			{0, -1},
			true
		);
	}

	void Environment::chart_ui() noexcept
	{
		ui::capsule::window(
			"##ClimateCharts",
			ui::capsule::Position::Bottom_left,
			[this] {
				if (ImGui::BeginTabBar("ClimateChartTabs"))
				{
					if (ImGui::BeginTabItem("柱状图"))
					{
						draw_bars();
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("对比表"))
					{
						draw_comparison_table();
						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}
			},
			{0, -1},
			true
		);
	}

	Climate Environment::generate_outdoor_climate(double sim_time) noexcept
	{
		const double hour_of_day = std::fmod(sim_time / 3600.0, 24.0);

		auto daily_variation = [hour_of_day](std::pair<float, float> range, double peak_hour) {
			const double radians = glm::radians((hour_of_day - peak_hour) / 24.0 * 360.0);
			const float variation = (std::cos(radians) + 1.0) / 2.0f;
			return range.first + (range.second - range.first) * variation;
		};

		Climate outdoor_climate;
		outdoor_climate.temperature = daily_variation(temp_range, max_temp_hour);
		outdoor_climate.humidity = daily_variation(humidity_range, max_humidity_hour);
		outdoor_climate.pm2_5 = daily_variation(pm25_range, pm25_peak_hour);
		outdoor_climate.carbon_oxide = daily_variation(co_range, co_peak_hour);

		return outdoor_climate;
	}

	Environment::Update_result Environment::update(double sim_time) noexcept
	{
		static thread_local std::mt19937 rng{std::random_device{}()};
		std::normal_distribution<double> noise_dist(1.0, 0.3);

		const double delta_sim_time =
			glm::mod(sim_time - prev_sim_time.value_or(sim_time) + 86400.0, 86400.0);
		const double alpha = delta_sim_time / sim_tau;

		area_climates.at(Area::Exterior) = generate_outdoor_climate(sim_time);

		for (const auto& [src, dst] : links)
		{
			auto& src_climate = area_climates.at(src);
			auto& dst_climate = area_climates.at(dst);

			dst_climate.temperature = glm::mix(
				dst_climate.temperature,
				src_climate.temperature,
				glm::clamp(alpha * noise_dist(rng), 0.0, 1.0)
			);
			dst_climate.humidity = glm::mix(
				dst_climate.humidity,
				src_climate.humidity,
				glm::clamp(alpha * noise_dist(rng), 0.0, 1.0)
			);
			dst_climate.pm2_5 =
				glm::mix(dst_climate.pm2_5, src_climate.pm2_5, glm::clamp(alpha * noise_dist(rng), 0.0, 1.0));
			dst_climate.carbon_oxide = glm::mix(
				dst_climate.carbon_oxide,
				src_climate.carbon_oxide,
				glm::clamp(alpha * noise_dist(rng), 0.0, 1.0)
			);
		}

		prev_sim_time = sim_time;

		Update_result result;
		for (const auto& [area, climate] : area_climates)
		{
			if (climate.pm2_5 >= fire_pm25_threshold || climate.carbon_oxide >= fire_co_threshold)
				result.fire_alert = area;
		}
		if (area_climates.at(Area::Exterior).pm2_5 >= bad_air_pm25_threshold) result.bad_outdoor_air = true;

		return result;
	}

	void Environment::draw_comparison_table() const noexcept
	{
		if (!ImGui::BeginTable(
				"ComparisonTable",
				5,
				ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable
			))
			return;

		ImGui::TableSetupColumn("房间", ImGuiTableColumnFlags_WidthFixed, 100.0f);
		ImGui::TableSetupColumn("温度(°C)", ImGuiTableColumnFlags_WidthFixed, 90.0f);
		ImGui::TableSetupColumn("湿度(%)", ImGuiTableColumnFlags_WidthFixed, 90.0f);
		ImGui::TableSetupColumn("PM2.5", ImGuiTableColumnFlags_WidthFixed, 90.0f);
		ImGui::TableSetupColumn("CO(ppm)", ImGuiTableColumnFlags_WidthFixed, 90.0f);
		ImGui::TableHeadersRow();

		for (auto& [area, climate] : area_climates)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(area_names.at(area));
			ImGui::TableNextColumn();
			ImGui::TextColored(
				color_palette::get_temperature_color(climate.temperature),
				"%.1f",
				climate.temperature
			);
			ImGui::TableNextColumn();
			ImGui::TextColored(color_palette::get_humidity_color(climate.humidity), "%.1f", climate.humidity);
			ImGui::TableNextColumn();
			ImGui::TextColored(color_palette::get_pm25_color(climate.pm2_5), "%.1f", climate.pm2_5);
			ImGui::TableNextColumn();
			ImGui::TextColored(
				color_palette::get_co_color(climate.carbon_oxide),
				"%.2f",
				climate.carbon_oxide
			);
		}

		ImGui::EndTable();
	}

	void Environment::draw_bars() const noexcept
	{
		for (auto& [area, climate] : area_climates)
		{
			ImGui::SeparatorText(area_names.at(area));
			draw_bar(climate);
		}
	}

}  // namespace logic