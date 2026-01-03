#include "logic/furniture-controller.hpp"
#include "ui/capsule.hpp"

#include <glm/glm.hpp>
#include <imgui.h>
#include <ranges>

namespace logic
{
	void Furniture_controller::State::update(float delta_time) noexcept
	{
		const float target_time = opened ? config.max_animation_time : 0.0f;

		current_time += glm::sign(target_time - current_time) * delta_time;
		current_time = glm::clamp(current_time, 0.0f, config.max_animation_time);
	}

	const std::vector<Furniture_controller::Config> Furniture_controller::furniture_configs = {
		Config{
			   .max_animation_time = 54.0f / 24.0f,
			   .hud = true,
			   .fire_area = Area::Small_bedroom,
			   .display_name = "门1",
			   .animation_name = "Door1",
			   .node_name = "Door1-Handle"
		},
		Config{
			   .max_animation_time = 54.0f / 24.0f,
			   .hud = true,
			   .fire_area = Area::Kitchen,
			   .display_name = "门2",
			   .animation_name = "Door2",
			   .node_name = "Door2-Handle"
		},
		Config{
			   .max_animation_time = 54.0f / 24.0f,
			   .hud = true,
			   .fire_area = Area::Toilet,
			   .display_name = "门3",
			   .animation_name = "Door3",
			   .node_name = "Door3-Handle"
		},
		Config{
			   .max_animation_time = 54.0f / 24.0f,
			   .hud = true,
			   .fire_area = Area::Large_bedroom,
			   .display_name = "门4",
			   .animation_name = "Door4",
			   .node_name = "Door4-Handle"
		},
		Config{
			   .max_animation_time = 5.0f,
			   .hud = true,
			   .fire_area = Area::Living_room,
			   .display_name = "门5",
			   .animation_name = "Door5",
			   .node_name = "Door5-Handle"
		},
		Config{
			   .max_animation_time = 3.0f,
			   .hud = false,
			   .fire_area = std::nullopt,
			   .display_name = "左窗帘",
			   .animation_name = "CurtainLeft",
			   .node_name = "Left"
		},
		Config{
			   .max_animation_time = 3.0f,
			   .hud = false,
			   .fire_area = std::nullopt,
			   .display_name = "右窗帘",
			   .animation_name = "CurtainRight",
			   .node_name = "Right"
		},
	};

	std::expected<Furniture_controller, util::Error> Furniture_controller::create(
		const gltf::Model& model
	) noexcept
	{
		std::vector<State> furniture_states;

		for (const auto& config : furniture_configs)
		{
			const auto node_index_opt = model.find_node_by_name(config.node_name);
			if (!node_index_opt.has_value())
				return util::Error(std::format("Furniture node '{}' not found", config.node_name));

			furniture_states.push_back(
				State{
					.config = config,
					.node_index = *node_index_opt,
				}
			);
		}

		return Furniture_controller(std::move(furniture_states));
	}

	void Furniture_controller::control_ui() noexcept
	{
		const auto func = [this] {
			const auto on_color = IM_COL32(100, 255, 100, 200);
			const auto on_hover = IM_COL32(150, 255, 150, 255);
			const auto on_active = IM_COL32(50, 200, 50, 255);

			const auto off_color = IM_COL32(255, 100, 100, 200);
			const auto off_hover = IM_COL32(255, 150, 150, 255);
			const auto off_active = IM_COL32(200, 50, 50, 255);

			for (auto& furniture : furniture_states)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, furniture.opened ? on_color : off_color);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, furniture.opened ? on_hover : off_hover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, furniture.opened ? on_active : off_active);

				if (ui::capsule::button(std::format("\U000f0425##Button{}", furniture.node_index)))
					furniture.opened = !furniture.opened;

				ImGui::PopStyleColor(3);

				ImGui::AlignTextToFramePadding();
				ui::capsule::small_label(furniture.config.display_name);

				ImGui::NewLine();
			}
		};

		ui::capsule::window("##FurnitureControl", ui::capsule::Position::Bottom_left, func, {0, -1}, false);
	}

	void Furniture_controller::hud_ui(
		std::span<const glm::mat4> node_vertices,
		const render::Camera_matrices& camera_matrices
	) noexcept
	{
		const auto on_color = IM_COL32(100, 255, 100, 200);
		const auto on_hover = IM_COL32(150, 255, 150, 255);
		const auto on_active = IM_COL32(50, 200, 50, 255);

		const auto off_color = IM_COL32(255, 100, 100, 200);
		const auto off_hover = IM_COL32(255, 150, 150, 255);
		const auto off_active = IM_COL32(200, 50, 50, 255);

		for (auto& furniture :
			 furniture_states | std::views::filter([](const State& state) { return state.config.hud; }))
		{
			const auto& node_matrix = node_vertices[furniture.node_index];

			const auto& world_position = glm::vec3(node_matrix[3]);
			if (glm::distance(world_position, camera_matrices.eye_position) > max_hud_distance) continue;

			const auto clip_space_pos =
				camera_matrices.proj_matrix * camera_matrices.view_matrix * glm::vec4(world_position, 1.0f);
			if (clip_space_pos.w <= 0.0f) continue;

			const auto ndc_pos = clip_space_pos / clip_space_pos.w;
			if (ndc_pos.z < 0.0f || ndc_pos.z > 1.0f) continue;
			if (glm::abs(ndc_pos.x) > 0.7f || glm::abs(ndc_pos.y) > 0.7f) continue;

			const auto window_size = ImGui::GetIO().DisplaySize;
			const auto window_size_glm = glm::vec2(window_size.x, window_size.y);
			const auto screen_pos = (glm::vec2(ndc_pos.x * 0.5f, -ndc_pos.y * 0.5f) + 0.5f) * window_size_glm;

			ImGui::SetNextWindowPos({screen_pos.x, screen_pos.y}, ImGuiCond_Always, {0.5f, 0.5f});
			ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));

			const auto window_began = ImGui::Begin(
				std::format("##FurnitureHUD{}", furniture.node_index).c_str(),
				nullptr,
				ImGuiWindowFlags_NoDecoration
					| ImGuiWindowFlags_AlwaysAutoResize
					| ImGuiWindowFlags_NoFocusOnAppearing
			);

			ImGui::PopStyleColor(2);

			if (window_began)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, furniture.opened ? on_color : off_color);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, furniture.opened ? on_hover : off_hover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, furniture.opened ? on_active : off_active);
				ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 128));
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

				if (ui::capsule::button("\U000f0425##HUDButton")) furniture.opened = !furniture.opened;

				ImGui::PopStyleVar();
				ImGui::PopStyleColor(4);
			}
			ImGui::End();
		}
	}

	std::vector<gltf::Animation_key> Furniture_controller::update() noexcept
	{
		const float delta_time = ImGui::GetIO().DeltaTime;
		for (auto& furniture : furniture_states) furniture.update(delta_time);

		return furniture_states
			| std::views::transform([](const auto& state) {
				   return gltf::Animation_key{
					   .animation = state.config.animation_name,
					   .time = state.current_time
				   };
			   })
			| std::ranges::to<std::vector>();
	}

	void Furniture_controller::handle_fire_event(Area fire_area) noexcept
	{
		for (auto& furniture : furniture_states)
		{
			furniture.opened =
				!furniture.config.fire_area.has_value() || furniture.config.fire_area.value() != fire_area;
		}
	}
}