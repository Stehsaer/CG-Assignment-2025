#pragma once

#include "backend/sdl.hpp"
#include "gltf/model.hpp"
#include "logic/area.hpp"
#include "logic/camera-control.hpp"
#include "logic/environment.hpp"
#include "logic/light-controller.hpp"
#include "logic/time-controller.hpp"
#include "render/drawdata/light.hpp"
#include "render/param.hpp"

#include <glm/glm.hpp>
#include <glm/trigonometric.hpp>

class Logic
{
	/* Resources */

	gltf::Model model;

	/* Controllers */

	logic::Camera camera_control = {};

	logic::Time_controller time_controller = {};
	logic::Light_controller light_controller;

	logic::Environment environment = {};

	void update(const backend::SDL_context& context) noexcept;

	/* Render */

	std::tuple<render::Params, std::vector<gltf::Drawdata>, std::vector<render::drawdata::Light>>
	get_render_params(const backend::SDL_context& context) noexcept;

	/* UI & UI States */

	enum class Sidebar_tab
	{
		Light_control,
		Charts_view,
		Climate_control
	};

	struct Sidebar_tab_info
	{
		const char* icon;
		const char* hint;
	};

	struct Fire_alarm
	{
		logic::Area area;
		double time;
		bool active;
	};

	std::optional<Sidebar_tab> active_sidebar_tab = std::nullopt;
	static const std::map<Sidebar_tab, Sidebar_tab_info> sidebar_tab_icons;

	std::optional<Fire_alarm> fire_alarm = std::nullopt;

	const std::string device_name;
	const std::string driver_name;

	void render_ui() noexcept;
	void left_sidebar_ui() noexcept;
	void draw_debug_overlay() noexcept;

	/* Constructor*/

	Logic(
		gltf::Model model,
		logic::Light_controller light_controller,
		std::string device_name,
		std::string driver_name
	) :
		model(std::move(model)),
		light_controller(std::move(light_controller)),
		device_name(std::move(device_name)),
		driver_name(std::move(driver_name))
	{}

  public:

	Logic(const Logic&) = delete;
	Logic(Logic&&) = default;
	Logic& operator=(const Logic&) = delete;
	Logic& operator=(Logic&&) = delete;

	static std::expected<Logic, util::Error> create(
		const backend::SDL_context& context,
		const std::string& path
	) noexcept;

	std::tuple<render::Params, std::vector<gltf::Drawdata>, std::vector<render::drawdata::Light>> logic(
		const backend::SDL_context& context
	) noexcept;
};