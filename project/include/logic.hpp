#pragma once

#include "backend/sdl.hpp"
#include "gltf/model.hpp"
#include "logic/area.hpp"
#include "logic/camera-control.hpp"
#include "logic/environment.hpp"
#include "logic/furniture-controller.hpp"
#include "logic/light-controller.hpp"
#include "logic/time-controller.hpp"
#include "render/drawdata/light.hpp"
#include "render/param.hpp"

#include <glm/glm.hpp>
#include <glm/trigonometric.hpp>

class Logic
{
  public:

	struct Render_output
	{
		render::Params params;
		gltf::Drawdata main_drawdata;
		std::vector<render::drawdata::Light> light_drawdata_list;
	};

	///
	/// @brief Create Logic instance
	///
	/// @param context SDL backend context
	/// @return Logic instance or error
	///
	static std::expected<Logic, util::Error> create(const backend::SDL_context& context) noexcept;

	///
	/// @brief Execute per-frame logic and produce render output
	///
	/// @param context SDL backend context
	/// @return Render output including render params and drawdata
	///
	Render_output logic(const backend::SDL_context& context) noexcept;

  private:

	/* Resources */

	gltf::Model model;

	const uint32_t ceiling_node_index;

	/* Controllers & States */

	struct Fire_alarm
	{
		logic::Area area;
		double time;
		bool active;
	};

	enum class View_mode
	{
		Walk,
		Free,
		Cross_section
	};

	logic::Camera main_camera = {};
	logic::Free_camera free_camera = {};

	inline static const graphics::camera::view::Flying cross_section_camera = {
		.position = glm::dvec3(0.0, 16.0, 0.0),
		.angles = {.azimuth = glm::radians(0.0f), .pitch = glm::radians(-90.0f)},
		.up = glm::dvec3(0.0, 0.0, -1.0)
	};

	inline static const render::Primary_light_params cross_section_light = {
		.direction = {0.0, 1.0, 0.0},
		.intensity = glm::vec3(80000.0f)
	};

	logic::Time_controller time_controller = {};
	logic::Light_controller light_controller;
	logic::Furniture_controller furniture_controller;
	logic::Environment environment;

	std::optional<Fire_alarm> fire_alarm = std::nullopt;
	View_mode view_mode = View_mode::Walk;

	///
	/// @brief Update and produce render output
	///
	/// @param context SDL backend context
	/// @return Render output including render params and drawdata
	///
	Render_output update(const backend::SDL_context& context) noexcept;

	/* UI & UI States */

	enum class Sidebar_tab
	{
		Light_control,
		Charts_view,
		Climate_control,
		Furniture_control
	};

	struct Sidebar_tab_info
	{
		const char* icon;
		const char* hint;
	};

	std::optional<Sidebar_tab> active_sidebar_tab = std::nullopt;
	static const std::map<Sidebar_tab, Sidebar_tab_info> sidebar_tab_icons;

	const std::string device_name;
	const std::string driver_name;

	///
	/// @brief Render UI elements, including huds and bars
	///
	/// @param node_vertices Node transformation matrices from main drawdata.
	/// @param camera_matrices Current camera matrices
	///
	void render_ui(
		std::span<const glm::mat4> node_vertices,
		const render::Camera_matrices& camera_matrices
	) noexcept;

	///
	/// @brief Display sidebar UI (at bottom left corner)
	///
	///
	void sidebar_ui() noexcept;

	///
	/// @brief Display sidebar tab buttons & sidebar tabs
	///
	///
	void sidebar_ui_tabs() noexcept;

	///
	/// @brief Display sidebar camera mode button
	///
	///
	void sidebar_ui_camera() noexcept;

	///
	/// @brief Draw debug overlay (fps, device name, driver name)
	///
	///
	void draw_debug_overlay() noexcept;

	/* Constructor */

	Logic(
		gltf::Model model,
		logic::Light_controller light_controller,
		logic::Furniture_controller furniture_controller,
		logic::Environment environment,
		std::string device_name,
		std::string driver_name,
		uint32_t ceiling_node_index
	) :
		model(std::move(model)),
		ceiling_node_index(ceiling_node_index),
		light_controller(std::move(light_controller)),
		furniture_controller(std::move(furniture_controller)),
		environment(std::move(environment)),
		device_name(std::move(device_name)),
		driver_name(std::move(driver_name))
	{}

  public:

	Logic(const Logic&) = delete;
	Logic(Logic&&) noexcept = default;
	Logic& operator=(const Logic&) = delete;
	Logic& operator=(Logic&&) = delete;
};