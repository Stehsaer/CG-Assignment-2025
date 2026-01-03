#include "logic.hpp"
#include "backend/loop.hpp"
#include "backend/sdl.hpp"
#include "logic/area.hpp"
#include "logic/light-controller.hpp"
#include "render/drawdata/light.hpp"
#include "render/param.hpp"
#include "ui/capsule.hpp"

#include <SDL3/SDL_properties.h>
#include <cstdint>
#include <imgui.h>
#include <implot.h>
#include <string>

static std::expected<gltf::Model, util::Error> create_scene_from_model(
	const backend::SDL_context& context,
	const std::string& path
) noexcept
{
	auto gltf_load_result = backend::display_until_task_done(
		context,
		std::async(std::launch::async, gltf::load_tinygltf_model_from_file, std::ref(path)),
		[] {
			ImGui::Text("加载模型...");
			ImGui::ProgressBar(-ImGui::GetTime(), ImVec2(300.0f, 0.0f));
		}
	);
	if (!gltf_load_result) return gltf_load_result.error().forward("Load tinygltf model failed");

	std::atomic<gltf::Model::Load_progress> load_progress;

	auto future = std::async(std::launch::async, [&context, &gltf_load_result, &load_progress]() {
		return gltf::Model::from_tinygltf(
			context.device,
			*gltf_load_result,
			gltf::Sampler_config{.anisotropy = 4.0f},
			{.color_mode = gltf::Color_compress_mode::RGBA8_BC3,
			 .normal_mode = gltf::Normal_compress_mode::RGn_BC5},
			std::ref(load_progress)
		);
	});

	auto gltf_result = backend::display_until_task_done(context, std::move(future), [&load_progress] {
		const auto current = load_progress.load();

		switch (current.stage)
		{
		case gltf::Model::Load_stage::Node:
			ImGui::Text("解析节点树...");
			break;
		case gltf::Model::Load_stage::Mesh:
			ImGui::Text("分析并优化网格...");
			break;
		case gltf::Model::Load_stage::Material:
			ImGui::Text("压缩材质...");
			break;
		case gltf::Model::Load_stage::Animation:
			ImGui::Text("解析动画...");
			break;
		case gltf::Model::Load_stage::Skin:
			ImGui::Text("解析皮肤...");
			break;
		case gltf::Model::Load_stage::Postprocess:
			ImGui::Text("处理中...");
			break;
		}

		ImGui::ProgressBar(
			current.progress < 0 ? (-ImGui::GetTime()) : current.progress,
			ImVec2(300.0f, 0.0f)
		);
	});
	if (!gltf_result) return gltf_result.error().forward("Load gltf model failed");

	return gltf_result;
}

std::expected<Logic, util::Error> Logic::create(
	const backend::SDL_context& context,
	const std::string& path
) noexcept
{
	auto model = create_scene_from_model(context, path);
	if (!model) return model.error().forward("Load 3D model failed");

	auto light_controller = logic::Light_controller::create(context.device, *model);
	if (!light_controller) return light_controller.error().forward("Create light controller failed");

	auto furniture_controller = logic::Furniture_controller::create(*model);
	if (!furniture_controller)
		return furniture_controller.error().forward("Create furniture controller failed");

	const auto prop = SDL_GetGPUDeviceProperties(context.device);
	if (prop == 0) return util::Error("Get SDL GPU device properties failed");

	const std::string device_name = SDL_GetStringProperty(prop, SDL_PROP_GPU_DEVICE_NAME_STRING, "Unknown");
	const std::string driver_name =
		SDL_GetStringProperty(prop, SDL_PROP_GPU_DEVICE_DRIVER_NAME_STRING, "Unknown");
	const std::string driver_version =
		SDL_GetStringProperty(prop, SDL_PROP_GPU_DEVICE_DRIVER_VERSION_STRING, "Unknown");

	SDL_DestroyProperties(prop);

	const auto ceiling_node_index = model->find_node_by_name("Ceiling");
	if (!ceiling_node_index.has_value()) return util::Error("Ceiling node not found in the model");

	return Logic(
		std::move(*model),
		std::move(*light_controller),
		std::move(*furniture_controller),
		device_name,
		std::format("{} ({})", driver_name, driver_version),
		*ceiling_node_index
	);
}

const std::map<Logic::Sidebar_tab, Logic::Sidebar_tab_info> Logic::sidebar_tab_icons = {
	{Sidebar_tab::Light_control,     {.icon = "\U000f06e8", .hint = "灯光控制"}}, // Light bulb icon
	{Sidebar_tab::Charts_view,       {.icon = "\uf201", .hint = "环境信息"}    }, // Chart icon
	{Sidebar_tab::Climate_control,   {.icon = "\U000f0393", .hint = "环境控制"}}, // Thermometer icon
	{Sidebar_tab::Furniture_control, {.icon = "\U000f0425", .hint = "家具控制"}}  // Couch icon
};

void Logic::sidebar_ui() noexcept
{
	ui::capsule::window("##LeftSidebar", ui::capsule::Position::Bottom_left, [this] {
		sidebar_ui_camera();
		ui::capsule::vertical_separator();
		sidebar_ui_tabs();
	});
}

void Logic::sidebar_ui_tabs() noexcept
{
	for (const auto& [tab, info] : sidebar_tab_icons)
	{
		const auto [icon, hint] = info;

		if (active_sidebar_tab.has_value())
		{
			if (active_sidebar_tab.value() == tab)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
				ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 255, 255, 200));
				if (ui::capsule::button(std::format("{}##SidebarTabActive{}", icon, static_cast<int>(tab))))
					active_sidebar_tab = std::nullopt;
				ImGui::PopStyleColor();
				ImGui::PopStyleVar();
			}
			else
			{
				if (ui::capsule::button(std::format("{}##SidebarTabInactive{}", icon, static_cast<int>(tab))))
					active_sidebar_tab = tab;
			}
		}
		else
		{
			if (ui::capsule::button(std::format("{}##SidebarTab{}", icon, static_cast<int>(tab))))
				active_sidebar_tab = tab;
		}

		ImGui::SetItemTooltip("%s", hint);
	}
}

void Logic::sidebar_ui_camera() noexcept
{
	switch (view_mode)
	{
	case View_mode::Walk:
		if (ui::capsule::button("\ue213 漫游视角", false)) view_mode = View_mode::Free;
		break;
	case View_mode::Free:
		if (ui::capsule::button("\uf1d9 自由视角", false)) view_mode = View_mode::Cross_section;
		break;
	case View_mode::Cross_section:
		if (ui::capsule::button("\U000F034D 剖面视角", false)) view_mode = View_mode::Walk;
		break;
	}
}

void Logic::render_ui(
	std::span<const glm::mat4> node_vertices,
	const render::Camera_matrices& camera_matrices
) noexcept
{
	// Bottom left sidebar
	sidebar_ui();

	// Debug overlay: fps, device name, driver name
	draw_debug_overlay();

	// Time controller UI, always visible
	time_controller.control_ui();

	// Furniture HUD UI
	furniture_controller.hud_ui(node_vertices, camera_matrices);

	// Active sidebar tab UI
	if (active_sidebar_tab.has_value())
	{
		switch (active_sidebar_tab.value())
		{
		case Sidebar_tab::Light_control:
			light_controller.control_ui();
			break;
		case Sidebar_tab::Charts_view:
			environment.chart_ui();
			break;
		case Sidebar_tab::Climate_control:
			environment.control_ui();
			break;
		case Sidebar_tab::Furniture_control:
			furniture_controller.control_ui();
			break;
		}
	}

	// Fire alarm UI
	if (fire_alarm && fire_alarm->active)
	{
		ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(192, 0, 0, 200));
		ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 0, 0, 255));
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
		ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(128, 0, 0, 255));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 64, 64, 255));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 128, 128, 255));

		ui::capsule::window("##FireAlarm", ui::capsule::Position::Top_center, [this] {
			ImGui::AlignTextToFramePadding();
			ui::capsule::label(
				std::format("\U000F0238 {}火灾警报！", logic::area_names.at(fire_alarm->area))
			);
			if (ui::capsule::button("\uf00c")) fire_alarm->active = false;
		});

		ImGui::PopStyleColor(6);
	}
}

void Logic::draw_debug_overlay() noexcept
{
	auto drawlist = ImGui::GetBackgroundDrawList();

	const auto draw_text = [drawlist](const std::string& text, glm::vec2 position, float font_size) {
		drawlist->AddText(
			ImGui::GetFont(),
			font_size,
			{position.x + 1, position.y + 1},
			IM_COL32(0, 0, 0, 255),
			text.c_str()
		);
		drawlist->AddText(
			ImGui::GetFont(),
			font_size,
			{position.x, position.y},
			IM_COL32(255, 255, 255, 255),
			text.c_str()
		);
	};

	const auto fps_string =
		std::format("{:.1f} FPS ({:.1f} ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

	draw_text(fps_string, {10.0f, 10.0f}, 24.0f);
	draw_text(device_name, {10.0f, 40.0f}, 16.0f);
	draw_text(driver_name, {10.0f, 60.0f}, 16.0f);
}

Logic::Render_output Logic::update(const backend::SDL_context& context) noexcept
{
	const auto camera_matrices = [&]() {
		switch (view_mode)
		{
		case View_mode::Walk:
			return main_camera.update(free_camera.update(context, false));
		case View_mode::Free:
			return main_camera.update(free_camera.update(context, true));
		case View_mode::Cross_section:
			return main_camera.update(cross_section_camera);
		}
		return render::Camera_matrices{};
	}();

	const auto sim_time = time_controller.update();
	const auto env_update_result = environment.update(sim_time);
	const auto animation_keys = furniture_controller.update();

	if (env_update_result.fire_alert)
	{
		if (!fire_alarm)
		{
			fire_alarm = {.area = *env_update_result.fire_alert, .time = sim_time, .active = true};
			light_controller.handle_fire_event();
			furniture_controller.handle_fire_event(*env_update_result.fire_alert);
		}
	}
	else
	{
		if (fire_alarm && !fire_alarm->active) fire_alarm = std::nullopt;
	}

	/* Generate render params */

	std::vector<std::pair<uint32_t, float>> emission_overrides = light_controller.get_emission_overrides();
	const auto [primary_light_param, ambient_light_param] = time_controller.get_sun_params();

	std::vector<uint32_t> hidden_nodes;
	if (view_mode == View_mode::Cross_section) hidden_nodes.push_back(ceiling_node_index);

	auto main_drawdata =
		model.generate_drawdata(glm::mat4(1.0f), animation_keys, emission_overrides, hidden_nodes);

	auto light_drawdata_list = light_controller.get_light_drawdata(main_drawdata);

	const render::Params params{
		.camera = camera_matrices,
		.primary_light = view_mode == View_mode::Cross_section ? cross_section_light : primary_light_param,
		.ambient = ambient_light_param
	};

	return {
		.params = params,
		.main_drawdata = std::move(main_drawdata),
		.light_drawdata_list = std::move(light_drawdata_list)
	};
}

Logic::Render_output Logic::logic(const backend::SDL_context& context) noexcept
{
	auto render_results = update(context);
	render_ui(render_results.main_drawdata.node_matrices, render_results.params.camera);
	return render_results;
}
