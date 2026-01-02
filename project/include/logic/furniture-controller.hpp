#pragma once

#include "area.hpp"
#include "gltf/model.hpp"
#include "util/error.hpp"

namespace logic
{
	class Furniture_controller
	{
	  public:

		static std::expected<Furniture_controller, util::Error> create(const gltf::Model& model) noexcept;

		void control_ui() noexcept;
		void hud_ui() noexcept;

		void update() noexcept;

		void handle_fire_event() noexcept;
	};
}