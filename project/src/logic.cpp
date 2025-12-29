#include "logic.hpp"
#include "backend/sdl.hpp"
#include "render/drawdata/light.hpp"
#include "render/param.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <imgui.h>
#include <implot.h>
#include <ranges>
#include <string>

void Logic::light_control_ui() noexcept
{
	ImGui::SeparatorText("光照");

	ImGui::SliderAngle("方位角", &light_azimuth, -180.0, 180.0);
	ImGui::SliderAngle("高度角", &light_pitch, -89.0, 89.0);

	ImGui::Separator();
	ImGui::Checkbox("脏镜头",&use_bloom_mask);
	ImGui::Separator();
}

void Logic::statistic_display_ui() const noexcept
{
	const auto& io = ImGui::GetIO();

	ImGui::SeparatorText("统计");

	ImGui::Text("Res: (%.0f, %.0f)", io.DisplaySize.x, io.DisplaySize.y);
	ImGui::Text("FPS: %.1f FPS", io.Framerate);
}

void Logic::animation_control_ui() noexcept
{
	ImGui::SeparatorText("动画");

	ImGui::Separator();

	ImGui::SliderFloat("左窗帘", &curtain_left_animation.target, 0.0f, 1.0f, "%.2f");
	ImGui::Text("当前位置: %.2f", curtain_left_animation.current);

	ImGui::SliderFloat("右窗帘", &curtain_right_animation.target, 0.0f, 1.0f, "%.2f");
	ImGui::Text("当前位置: %.2f", curtain_right_animation.current);
}

void Logic::light_source_control_ui() noexcept
{
	ImGui::SeparatorText("光源");
	for (auto& [_, light] : light_groups)
	{
		ImGui::Checkbox(light.display_name.c_str(), &light.enabled);
	}
}

void Logic::climate_control_ui() noexcept
{
	climate_viewer.control_ui();
}

static void draw_world_label(
	const render::Camera_matrices& camera_matrices,
	glm::vec3 world_position,
	const char* text
) noexcept
{
	const auto clip =
		camera_matrices.proj_matrix * camera_matrices.view_matrix * glm::vec4(world_position, 1.0f);
	if (clip.w <= 0.0f) return;

	const auto ndc = glm::vec3(clip) / clip.w;
	if (!glm::all(glm::lessThan(glm::abs(glm::vec2(ndc)), glm::vec2(1.0f)))) return;

	const auto viewport_size = ImGui::GetIO().DisplaySize;
	const auto uv = glm::fma(glm::vec2(ndc), glm::vec2(0.5f, -0.5f), glm::vec2(0.5f));
	const auto puv = uv * glm::vec2(viewport_size.x, viewport_size.y);

	ImDrawList* draw_list = ImGui::GetForegroundDrawList();
	const ImVec2 center{puv.x, puv.y};
	const ImVec2 text_size = ImGui::CalcTextSize(text);

	constexpr float pad_x = 8.0f;
	constexpr float pad_y = 4.0f;
	const ImVec2 rect_min{center.x - text_size.x * 0.5f - pad_x, center.y - text_size.y * 0.5f - pad_y};
	const ImVec2 rect_max{center.x + text_size.x * 0.5f + pad_x, center.y + text_size.y * 0.5f + pad_y};

	draw_list->AddRectFilled(rect_min, rect_max, IM_COL32(0, 0, 0, 160), 4.0f);
	draw_list->AddRect(rect_min, rect_max, IM_COL32(255, 255, 255, 90), 4.0f);
	draw_list->AddText(
		ImVec2(center.x - text_size.x * 0.5f, center.y - text_size.y * 0.5f),
		IM_COL32(255, 255, 255, 230),
		text
	);
}

void Logic::Animation_state::update(float dt) noexcept
{
	const float lerp = glm::clamp(dt * lerp_factor, 0.0f, 1.0f);
	current = glm::mix(current, target, lerp);
}

float Logic::Animation_state::get_time() const noexcept
{
	return current * max_time;
}

std::expected<Logic, util::Error> Logic::create(SDL_GPUDevice* device, const gltf::Model& model) noexcept
{
	Logic logic;

	const auto door1_node_index = model.find_node_by_name("Door1-Handle");
	const auto door2_node_index = model.find_node_by_name("Door2-Handle");
	const auto door3_node_index = model.find_node_by_name("Door3-Handle");
	const auto door4_node_index = model.find_node_by_name("Door4-Handle");
	const auto door5_node_index = model.find_node_by_name("Door5-Handle");
	const auto ceiling_node_index = model.find_node_by_name("Ceiling");
	const auto curtain_left_node_index = model.find_node_by_name("Left");
	const auto curtain_right_node_index = model.find_node_by_name("Right");

	if (!door1_node_index || !door2_node_index || !door3_node_index || !door4_node_index || !door5_node_index)
		return util::Error("Failed to find door handle nodes by name");
	if (!ceiling_node_index) return util::Error("Failed to find ceiling node by name");
	if (!curtain_left_node_index || !curtain_right_node_index)
		return util::Error("Failed to find curtain nodes by name");

	logic.door1_node_index = *door1_node_index;
	logic.door2_node_index = *door2_node_index;
	logic.door3_node_index = *door3_node_index;
	logic.door4_node_index = *door4_node_index;
	logic.door5_node_index = *door5_node_index;
	logic.ceiling_node_index = *ceiling_node_index;

	auto load_lights_result = logic::load_light_groups(device, model);
	if (!load_lights_result) return load_lights_result.error().forward("Load light groups failed");
	logic.light_groups = std::move(*load_lights_result);
	logic.curtain_left_node_index = *curtain_left_node_index;
	logic.curtain_right_node_index = *curtain_right_node_index;

	return logic;
}

Logic::Animation_metric Logic::Animation_metric::from(
	size_t animation_index,
	const render::Camera_matrices& camera_matrices,
	glm::mat4 handle_matrix,
	glm::vec3 forward_dir
) noexcept
{
	const auto handle_position = glm::vec3(handle_matrix[3]);
	const auto door_ray = handle_position - camera_matrices.eye_position;
	const auto door_dir = glm::normalize(door_ray);
	const auto distance = glm::length(door_ray);
	const float door_dot = glm::dot(glm::normalize(forward_dir), door_dir);

	const auto handle_ndc_h =
		camera_matrices.proj_matrix * camera_matrices.view_matrix * glm::vec4(handle_position, 1.0f);
	const auto handle_ndc = handle_ndc_h / handle_ndc_h.w;
	const auto handle_uv = glm::fma(glm::vec2(handle_ndc), glm::vec2(0.5, -0.5), glm::vec2(0.5));
	const auto viewport_size = ImGui::GetIO().DisplaySize;
	const auto puv = handle_uv * glm::vec2(viewport_size.x, viewport_size.y);

	return {.animation_index = animation_index, .distance = distance, .dot = door_dot, .puv = puv};
}

std::tuple<render::Params, std::vector<gltf::Drawdata>, std::vector<render::drawdata::Light>> Logic::logic(
	const backend::SDL_context& context,
	const gltf::Model& model
) noexcept
{
	struct Door_ui_state
	{
		std::string name;
		uint32_t node_index;
		float* position;
		float* target;
		bool looking_at = false;
		glm::vec2 screen_pos{0.0f};
	};

	camera_control.update_motion(context);
	const auto camera_matrices = camera_control.update_and_get_matrices();
	const auto inv_view_mat = glm::inverse(camera_matrices.view_matrix);
	const auto forward_dir = glm::normalize(glm::vec3(-inv_view_mat[2]));
	const float look_cos_threshold = 0.9f;
	const float look_distance_threshold = 3.0f;

	// 获取delta time
	const float dt = ImGui::GetIO().DeltaTime;

	// 更新昼夜循环
	day_night_cycle.update(dt);

	// 如果启用自动光照控制，应用光照参数
	if (day_night_cycle.is_enabled() && day_night_cycle.is_auto_light_control())
	{
		const auto light_params = day_night_cycle.get_light_params();
		light_azimuth = light_params.azimuth;
		light_pitch = light_params.pitch;
		light_color = light_params.color;
		light_intensity = light_params.intensity * 80000;
		ambient_intensity = light_params.ambient_intensity * 50;
	}

	// 更新气候数据
	if (day_night_cycle.is_enabled())
	{
		climate_viewer.update_from_time_of_day(day_night_cycle.get_time_of_day());
	}

	// 火灾检测 - 根据区域危险状态控制门窗
	// 区域划分映射关系：
	// Room_zone::Bedroom1 (0)   -> door1
	// Room_zone::Kitchen (1)    -> door2
	// Room_zone::Bathroom (2)   -> door3
	// Room_zone::Bedroom2 (3)   -> door4
	// Room_zone::LivingRoom (4) -> door5 + curtain_left + curtain_right
	//
	// 逻辑：起火区域关闭门窗（隔离火源），安全区域打开门窗（便于疏散）
	if (climate_viewer.is_fire_alarm_active())
	{
		using logic::Room_zone;

		// 卧室1 (door1)
		const bool bedroom1_danger = climate_viewer.is_zone_in_danger(Room_zone::Bedroom1);
		door1_animation.target = bedroom1_danger ? 0.0f : 1.0f;

		// 厨房 (door2)
		const bool kitchen_danger = climate_viewer.is_zone_in_danger(Room_zone::Kitchen);
		door2_animation.target = kitchen_danger ? 0.0f : 1.0f;

		// 卫生间 (door3)
		const bool bathroom_danger = climate_viewer.is_zone_in_danger(Room_zone::Bathroom);
		door3_animation.target = bathroom_danger ? 0.0f : 1.0f;

		// 卧室2 (door4)
		const bool bedroom2_danger = climate_viewer.is_zone_in_danger(Room_zone::Bedroom2);
		door4_animation.target = bedroom2_danger ? 0.0f : 1.0f;

		// 客厅 (door5 + curtains)
		const bool livingroom_danger = climate_viewer.is_zone_in_danger(Room_zone::LivingRoom);
		door5_animation.target = livingroom_danger ? 0.0f : 1.0f;
		curtain_left_animation.target = livingroom_danger ? 0.0f : 1.0f;
		curtain_right_animation.target = livingroom_danger ? 0.0f : 1.0f;
	}

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 work_pos = viewport->WorkPos;
	const ImVec2 work_size = viewport->WorkSize;

	constexpr float top_margin = 20.0f;
	constexpr float side_margin = 8.0f;

	ImGui::SetNextWindowPos(
		ImVec2(work_pos.x + side_margin, work_pos.y + top_margin),
		ImGuiCond_Always,
		ImVec2(0.0f, 0.0f)
	);
	if (ImGui::Begin(
			"设置",
			nullptr,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
		))
	{
		day_night_cycle.control_ui();
		light_control_ui();
		camera_control.camera_control_ui();
		section_view.control_ui();
		statistic_display_ui();
		animation_control_ui();
		light_source_control_ui();
	}
	ImGui::End();

	ImGui::SetNextWindowPos(
		ImVec2(work_pos.x + work_size.x - side_margin, work_pos.y + top_margin),
		ImGuiCond_Always,
		ImVec2(1.0f, 0.0f)
	);
	if (ImGui::Begin(
			"室内环境监控系统",
			nullptr,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
		))
	{
		climate_control_ui();
	}
	ImGui::End();

	const auto light_direction = glm::vec3(
		glm::cos(light_pitch) * glm::sin(light_azimuth),
		glm::sin(light_pitch),
		glm::cos(light_pitch) * glm::cos(light_azimuth)
	);

	const std::array animation_targets = std::to_array({
		std::make_pair(door1_node_index, std::ref(door1_animation)),
		std::make_pair(door2_node_index, std::ref(door2_animation)),
		std::make_pair(door3_node_index, std::ref(door3_animation)),
		std::make_pair(door4_node_index, std::ref(door4_animation)),
		std::make_pair(door5_node_index, std::ref(door5_animation)),
	});

	const std::array curtain_targets = std::to_array({
		std::ref(curtain_left_animation),
		std::ref(curtain_right_animation),
	});

	std::vector<gltf::Animation_key> animation_keys;
	for (const auto& [_, animation_state] : animation_targets)
	{
		animation_keys.push_back(
			gltf::Animation_key{.animation = animation_state.key, .time = animation_state.get_time()}
		);
	}
	for (const auto& animation_state : curtain_targets)
	{
		animation_keys.push_back(
			gltf::Animation_key{
				.animation = animation_state.get().key,
				.time = animation_state.get().get_time()
			}
		);
	}

	// 处理剖面图模式切换
	static bool prev_section_view_enabled = false;
	if (section_view.is_enabled() != prev_section_view_enabled)
	{
		section_view.toggle(camera_control, light_azimuth, light_pitch);
		prev_section_view_enabled = section_view.is_enabled();
	}

	// 处理节点隐藏切换
	static bool prev_hide_nodes_enabled = false;
	if (section_view.is_hide_nodes_enabled() != prev_hide_nodes_enabled)
	{
		// section_view.toggle_hide_nodes(model);
		prev_hide_nodes_enabled = section_view.is_hide_nodes_enabled();
	}

	std::vector<uint32_t> hidden_nodes;
	if (prev_hide_nodes_enabled) hidden_nodes.push_back(ceiling_node_index);
	std::vector<std::pair<uint32_t, float>> emission_overrides;
	for (const auto& light_group : light_groups | std::views::values)
	{
		emission_overrides.append_range(
			light_group.emission_nodes
			| std::views::transform([mult = light_group.enabled ? 1.0f : 0.0f](uint32_t node_index) {
				  return std::make_pair(node_index, mult);
			  })
		);
	}

	auto main_drawdata =
		model.generate_drawdata(glm::mat4(1.0f), animation_keys, emission_overrides, hidden_nodes);

	// 剖面图模式：在每个区域显示名称（用门把手节点作为区域锚点）
	if (section_view.is_enabled())
	{
		auto label_at_node = [&](uint32_t node_index, const char* label) {
			if (node_index >= main_drawdata.node_matrices.size()) return;
			const auto world_pos = glm::vec3(main_drawdata.node_matrices[node_index][3]);
			draw_world_label(camera_matrices, world_pos, label);
		};

		// 区域与门窗对应关系：
		// door1 -> 卧室1, door2 -> 厨房, door3 -> 卫生间, door4 -> 卧室2, door5(+窗帘) -> 客厅
		label_at_node(door1_node_index, "卧室1");
		label_at_node(door2_node_index, "厨房");
		label_at_node(door3_node_index, "卫生间");
		label_at_node(door4_node_index, "卧室2");
		label_at_node(door5_node_index, "客厅");
	}

	for (const auto& [_, animation_state] : animation_targets) animation_state.update(dt);
	for (const auto& animation_state : curtain_targets) animation_state.get().update(dt);

	auto animation_metrics =
		animation_targets
		| std::views::keys
		| std::views::enumerate
		| std::views::transform([&main_drawdata, &camera_matrices, &forward_dir](auto pair) {
			  const auto [idx, node_index] = pair;
			  const auto handle_matrix = main_drawdata.node_matrices[node_index];
			  return Animation_metric::from(idx, camera_matrices, handle_matrix, forward_dir);
		  })
		| std::views::filter([&look_cos_threshold, &look_distance_threshold](const auto& metric) {
			  return metric.dot > look_cos_threshold && metric.distance < look_distance_threshold;
		  })
		| std::ranges::to<std::vector>();

	std::ranges::sort(animation_metrics, std::less(), &Animation_metric::distance);
	if (!animation_metrics.empty())
	{
		const auto& closest_metric = animation_metrics.front();
		auto& target = animation_targets[closest_metric.animation_index];

		ImGui::SetNextWindowPos(
			ImVec2(closest_metric.puv.x, closest_metric.puv.y),
			ImGuiCond_Always,
			ImVec2(0.5f, 0.5f)
		);

		const bool is_open = target.second.current > 0.99;
		const bool is_closed = target.second.current < 0.01f;

		if (is_open || is_closed)
		{
			if (ImGui::Begin(
					"门提示",
					nullptr,
					ImGuiWindowFlags_AlwaysAutoResize
						| ImGuiWindowFlags_NoTitleBar
						| ImGuiWindowFlags_NoBackground
				))
			{
				ImGui::PushFont(nullptr, 30.0f);
				if (is_open && ImGui::Button("关门"))
				{
					target.second.target = 0.0f;
				}
				else if (is_closed && ImGui::Button("开门"))
				{
					target.second.target = 1.0f;
				}
				ImGui::PopFont();
			}
			ImGui::End();
		}
	}

	auto light_drawdata_list =
		light_groups
		| std::views::values
		| std::views::filter(&logic::Light_group::enabled)
		| std::views::transform(&logic::Light_group::lights)
		| std::views::join
		| std::views::transform([&main_drawdata](const logic::Light_source& light) {
			  return render::drawdata::Light::from(
				  main_drawdata.node_matrices[light.node_index],
				  glm::mat4(1.0),
				  light.light,
				  light.volume
			  );
		  })
		| std::ranges::to<std::vector>();

	std::vector<gltf::Drawdata> drawdata_list;
	drawdata_list.emplace_back(std::move(main_drawdata));

	const render::Primary_light_params primary_light{
		.direction = light_direction,
		.intensity = light_color * light_intensity,
	};

	const render::Bloom_params bloom_params{
		.bloom_attenuation = bloom_attenuation,
		.bloom_strength = bloom_strength,
	};

	const render::Shadow_params shadow_params{
		.csm_linear_blend = csm_linear_blend,
	};

	const render::Params params{
		.aa_mode = aa_mode,
		.camera = camera_matrices,
		.primary_light = primary_light,
		.ambient = {.intensity = glm::vec3(ambient_intensity)},
		.bloom = bloom_params,
		.shadow = shadow_params,
		.function_mask = {.use_bloom_mask = use_bloom_mask}
	};

	return std::make_tuple(params, std::move(drawdata_list), std::move(light_drawdata_list));
}
