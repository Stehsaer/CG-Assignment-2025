#include "logic/light-controller.hpp"

#include "asset/light-volume.hpp"
#include "render/light-volume.hpp"
#include "ui/capsule.hpp"
#include "util/asset.hpp"
#include "zip/zip.hpp"

#include <SDL3/SDL_gpu.h>
#include <imgui.h>
#include <nlohmann/json.hpp>

namespace logic
{
	static std::expected<nlohmann::json, util::Error> load_json() noexcept
	{
		const auto json_text =
			util::get_asset(resource_asset::light_volume, "light-volume-table.json")
				.and_then(zip::Decompress())
				.transform([](std::vector<std::byte> data) {
					return std::string(reinterpret_cast<const char*>(data.data()), data.size());
				});
		if (!json_text) return json_text.error().forward("Can't find light volume table config");

		try
		{
			nlohmann::json json = nlohmann::json::parse(*json_text);
			if (!json.is_object()) return util::Error("Light group JSON is not an object");
			return json;
		}
		catch (const nlohmann::json::parse_error& e)
		{
			return util::Error(std::format("Parse light group JSON failed: {}", e.what()));
		}
	}

	static std::expected<std::map<std::string, Light_group>, util::Error> get_groups(
		const nlohmann::json& json,
		const gltf::Model& model
	) noexcept
	{
		try
		{
			if (!json.is_array()) return util::Error("JSON root is not an array");
			std::map<std::string, Light_group> groups;

			for (const auto& [key, value] : json.items())
			{
				if (!value.is_object())
					return util::Error(std::format("Light group index {} is not an object", key));
				if (!value.contains("name"))
					return util::Error(std::format("Light group index {} has no name", key));
				if (!value.contains("display"))
					return util::Error(std::format("Light group index {} has no display name", key));

				const auto group_name = value["name"].get<std::string>();

				std::vector<uint32_t> emission_nodes;
				if (value.contains("emission_nodes"))
				{
					const auto nodes = value["emission_nodes"];
					if (!nodes.is_array())
						return util::Error(
							std::format("Light group '{}' emission_nodes is not an array", group_name)
						);

					for (const auto& [_, node_name] : nodes.items())
					{
						if (!node_name.is_string())
							return util::Error(
								std::format(
									"Light group '{}' emission_nodes has non-string entry",
									group_name
								)
							);
						const auto find_node_result = model.find_node_by_name(node_name.get<std::string>());
						if (!find_node_result)
							return util::Error(
								std::format(
									"Can't find emission node '{}' for light group '{}'",
									node_name.get<std::string>(),
									group_name
								)
							);
						emission_nodes.push_back(*find_node_result);
					}
				}

				auto group = Light_group{
					.display_name = value["display"].get<std::string>(),
					.lights = {},
					.emission_nodes = std::move(emission_nodes),
					.enabled = true,
				};

				groups.emplace(value["name"].get<std::string>(), group);
			}

			return groups;
		}
		catch (const nlohmann::json::type_error& e)
		{
			return util::Error(std::format("Parse light group JSON failed: {}", e.what()));
		}
	}

	static std::expected<void, util::Error> assign_lights_to_groups(
		SDL_GPUDevice* device,
		std::map<std::string, Light_group>& groups,
		const nlohmann::json& json,
		const gltf::Model& model
	) noexcept
	{
		if (!json.is_array()) return util::Error("JSON root is not an array");

		for (const auto& [idx_str, value] : json.items())
		{
			if (!value.is_object())
				return util::Error(std::format("Light group index {} is not an object", idx_str));
			if (!value.contains("node_name") || !value.contains("group") || !value.contains("path"))
				return util::Error(std::format("Light group index {} is missing required fields", idx_str));

			const auto node_name = value["node_name"].get<std::string>();
			const auto group_name = value["group"].get<std::string>();
			const auto path = value["path"].get<std::string>();

			const auto find_light_result = model.find_light_by_name(node_name);
			if (!find_light_result)
				return util::Error(std::format("Can't find light '{}' in model", node_name));

			const auto find_group_it = groups.find(group_name);
			if (find_group_it == groups.end())
				return util::Error(std::format("Can't find light group '{}'", group_name));

			auto volume =
				util::get_asset(resource_asset::light_volume, path)
					.and_then(zip::Decompress())
					.and_then(wavefront::parse_raw)
					.and_then(
						[device, &node_name](const wavefront::Object& raw_model)
							-> std::expected<render::Light_volume, util::Error> {
							return render::Light_volume::from_model(
								device,
								raw_model,
								std::format("Light volume '{}'", node_name)
							);
						}
					);
			if (!volume)
				return volume.error().forward(std::format("Load light volume '{}' failed", node_name));

			const auto [node_index, light] = *find_light_result;
			find_group_it->second.lights.push_back(
				Light_source{
					.volume =
						std::make_shared<render::Light_volume>(std::move(*volume)),  // to be filled later
					.light = light,
					.node_index = node_index
				}
			);
		}

		return {};
	}

	std::expected<Light_controller, util::Error> Light_controller::create(
		SDL_GPUDevice* device,
		const gltf::Model& model
	) noexcept
	{
		auto json_result = load_json();
		if (!json_result) return json_result.error().forward("Load light group JSON failed");
		auto json = std::move(*json_result);

		if (!json.contains("groups")) return util::Error("Light group JSON has no 'groups' field");
		auto groups_result = get_groups(json["groups"], model);
		if (!groups_result) return groups_result.error().forward("Parse light groups failed");
		auto groups = std::move(*groups_result);

		if (!json.contains("lights")) return util::Error("Light group JSON has no 'lights' field");
		const auto assign_result = assign_lights_to_groups(device, groups, json["lights"], model);
		if (!assign_result) return assign_result.error().forward("Assign lights to groups failed");

		return Light_controller(std::move(groups));
	}

	void Light_controller::control_ui() noexcept
	{
		ui::capsule::window(
			"##LightControl",
			ui::capsule::Position::Bottom_left,
			[this] {
				const auto light_on_icon = "\U000f0335";   // Light on icon
				const auto light_off_icon = "\U000f0e50";  // Light off icon

				for (auto& [group_name, group] : light_groups)
				{
					if (ui::capsule::button(
							std::format(
								"{}##LightGroupToggle{}",
								group.enabled ? light_on_icon : light_off_icon,
								group_name
							)
						))
						group.enabled = !group.enabled;

					ui::capsule::small_label(group.display_name);
					ImGui::NewLine();
				}
			},
			{0, -1}
		);
	}

	std::vector<std::pair<uint32_t, float>> Light_controller::get_emission_overrides() const noexcept
	{
		std::vector<std::pair<uint32_t, float>> emission_overrides;

		for (const auto& light_group : light_groups | std::views::values)
			emission_overrides.append_range(
				light_group.emission_nodes | std::views::transform([&light_group](uint32_t node_index) {
					return std::make_pair(node_index, light_group.enabled ? 1.0f : 0.0f);
				})
			);

		return emission_overrides;
	}

	std::vector<render::drawdata::Light> Light_controller::get_light_drawdata(
		const gltf::Drawdata& drawdata
	) const noexcept
	{
		return light_groups
			| std::views::values
			| std::views::filter(&logic::Light_group::enabled)
			| std::views::transform(&logic::Light_group::lights)
			| std::views::join
			| std::views::transform([&drawdata](const logic::Light_source& light) {
				   return render::drawdata::Light::from(
					   drawdata.node_matrices[light.node_index],
					   glm::mat4(1.0),
					   light.light,
					   light.volume
				   );
			   })
			| std::ranges::to<std::vector>();
	}

	void Light_controller::handle_fire_event() noexcept
	{
		for (auto& group : light_groups | std::views::values) group.enabled = true;
	}
}