#pragma once

#include "gltf/light.hpp"
#include "gltf/model.hpp"
#include "render/drawdata/light.hpp"
#include "render/light-volume.hpp"

#include <expected>
#include <map>
#include <memory>
#include <string>

namespace logic
{
	struct Light_source
	{
		std::shared_ptr<const render::Light_volume> volume;
		gltf::Light light;
		uint32_t node_index;
	};

	struct Light_group
	{
		std::string display_name;
		std::vector<Light_source> lights;
		std::vector<uint32_t> emission_nodes;
		bool enabled = true;
	};

	class Light_controller
	{
		std::map<std::string, logic::Light_group> light_groups;

		Light_controller(std::map<std::string, logic::Light_group> light_groups) :
			light_groups(std::move(light_groups))
		{}

	  public:

		static std::expected<Light_controller, util::Error> create(
			SDL_GPUDevice* device,
			const gltf::Model& model
		) noexcept;

		///
		/// @brief Light control UI
		///
		///
		void control_ui() noexcept;

		///
		/// @brief Get emission overrides for light groups
		/// @return List of node index and emission multiplier pairs
		///
		std::vector<std::pair<uint32_t, float>> get_emission_overrides() const noexcept;

		///
		/// @brief Get light drawdata from enabled light groups
		/// @param drawdata Main drawdata for node matrices
		/// @return List of light drawdata
		///
		std::vector<render::drawdata::Light> get_light_drawdata(
			const gltf::Drawdata& drawdata
		) const noexcept;

		///
		/// @brief Handle fire event by enabling all lights
		///
		///
		void handle_fire_event() noexcept;
	};
}