#include "logic.hpp"
#include "capsule-ui.hpp"
#include "geometry/primitive.hpp"
#include "geometry/vertex.hpp"
#include "pipeline/surface.hpp"
#include "util/as-byte.hpp"

#include <SDL3/SDL_gpu.h>
#include <format>
#include <imgui-knobs.h>
#include <imgui.h>
#include <ranges>
#include <utility>
#include <variant>

namespace logic
{
	std::pair<const char*, const char*> App::get_bottom_bar_tab_icon(BottomBarTab tab) noexcept
	{
		switch (tab)
		{
		case BottomBarTab::Edit:
			return {"\uf044", "编辑"};
		case BottomBarTab::DrawLine:
			return {"\U000f055e", "画线"};
		case BottomBarTab::DrawCircle:
			return {"\U000f0557", "画圆"};
		case BottomBarTab::DrawBezier:
			return {"\U000f0ae8", "画贝塞尔曲线"};
		case BottomBarTab::DrawCubicSpline:
			return {"\uee24", "画三次样条曲线"};
		}

		return {"", "==BUG=="};  // Should not reach here
	}

	std::expected<void, util::Error> App::imgui_frame(SDL_GPUDevice* device) noexcept
	{
		switch (mode)
		{
		case Mode::Line2D:
		{
			update_camera_2d();
			const auto vp_matrix =
				current_camera_2d.get_matrix(ImGui::GetIO().DisplaySize.x / ImGui::GetIO().DisplaySize.y);

			const auto new_tab = bottom_ui_2d(get_current_tab(draw_edit_state));
			auto new_state = handle_state_2d(device, std::move(draw_edit_state), new_tab, vp_matrix);
			if (!new_state.has_value()) return new_state.error().forward("Handle app state failed");
			draw_edit_state = std::move(new_state.value());

			break;
		}
		case Mode::Line3D:
		case Mode::Solid3D:
		{
			const auto& io = ImGui::GetIO();
			const auto screen_size = glm::vec2(io.DisplaySize.x, io.DisplaySize.y);
			const auto mouse_delta = glm::vec2(io.MouseDelta.x, io.MouseDelta.y);

			if (!io.WantCaptureMouse)
			{
				if (io.MouseDown[ImGuiMouseButton_Middle])
					camera_3d_view.angles =
						camera_3d_view.angles
							.rotate(glm::radians(180.0f), glm::radians(90.0f), screen_size, mouse_delta);

				camera_3d_view.distance *= glm::pow(1.2f, -io.MouseWheel);
			}

			bottom_ui_3d();

			break;
		}
		}

		performance_overlay();

		return {};
	}

	std::expected<void, util::Error> App::update_temp_buffer_2d(
		SDL_GPUDevice* device,
		std::span<const LineVertex> vertices
	) noexcept
	{
		if (const auto result = temp_buffer.write_transfer(device, std::as_bytes(vertices));
			!result.has_value())
			return result.error().forward("Write to temp buffer failed");

		temp_buffer_vertex_count = vertices.size();
		return {};
	}

	std::expected<void, util::Error> App::reset_temp_buffer_2d(
		SDL_GPUDevice* device [[maybe_unused]]
	) noexcept
	{
		temp_buffer_vertex_count = 0;
		return {};
	}

	std::expected<void, util::Error> App::rebuild_persistent_buffer_2d(SDL_GPUDevice* device) noexcept
	{
		size_t vertex_count = 0;
		std::vector<std::byte> vertex_data;
		std::vector<SDL_GPUIndirectDrawCommand> indirect_commands;

		const auto add_data =
			[&vertex_data, &indirect_commands, &vertex_count](std::span<const LineVertex> vertices) {
				if (vertices.empty()) return;

				vertex_data.append_range(util::as_bytes(vertices));
				indirect_commands.push_back(
					SDL_GPUIndirectDrawCommand{
						.num_vertices = uint32_t(vertices.size()),
						.num_instances = 1,
						.first_vertex = uint32_t(vertex_count),
						.first_instance = 0
					}
				);

				vertex_count += vertices.size();
			};

		for (const auto& line_entry : lines) add_data(std::span<const LineVertex>(line_entry.get()));
		for (const auto& circle_entry : circles) add_data(std::span<const LineVertex>(circle_entry.get()));
		for (const auto& bezier_entry : beziers) add_data(std::span<const LineVertex>(bezier_entry.get()));
		for (const auto& spline_entry : splines) add_data(std::span<const LineVertex>(spline_entry.get()));

		if (const auto write_result =
				persistent_vertex_buffer.write_transfer(device, std::span<const std::byte>(vertex_data));
			!write_result.has_value())
			return write_result.error().forward("Write to persistent buffer failed");

		if (const auto write_result =
				persistent_indirect_buffer
					.write_transfer(device, std::span<const std::byte>(util::as_bytes(indirect_commands)));
			!write_result.has_value())
			return write_result.error().forward("Write to persistent indirect buffer failed");

		persistent_buffer_indirect_count = indirect_commands.size();
		persistent_buffer_vertex_count = vertex_count;

		return {};
	}

	void App::upload_frame(const gpu::CopyPass& copy_pass) noexcept
	{
		switch (mode)
		{
		case Mode::Line2D:
			upload_frame_2d(copy_pass);
			break;
		default:
			break;
		}
	}

	void App::draw_frame(
		const gpu::GraphicsPipeline& line_pipeline,
		const pipeline::Surface& surface_pipeline,
		const gpu::CommandBuffer& command_buffer,
		const gpu::RenderPass& render_pass
	) noexcept
	{
		const auto& io = ImGui::GetIO();
		const auto aspect_ratio = io.DisplaySize.x / io.DisplaySize.y;

		if (io.DisplaySize.x < 100 || io.DisplaySize.y < 100) return;

		switch (mode)
		{
		case Mode::Line2D:
			draw_frame_2d(line_pipeline, command_buffer, render_pass);
			break;
		case Mode::Line3D:
		case Mode::Solid3D:
			surface_pipeline.draw(
				command_buffer,
				render_pass,
				{.vp_matrix = camera_3d_projection.matrix(aspect_ratio) * camera_3d_view.matrix(),
				 .control_points = bezier_3d_points},
				mode == Mode::Line3D
			);
		}
	}

	void App::upload_frame_2d(const gpu::CopyPass& copy_pass) noexcept
	{
		temp_buffer.copy_to_gpu(copy_pass);
		persistent_vertex_buffer.copy_to_gpu(copy_pass);
		persistent_indirect_buffer.copy_to_gpu(copy_pass);
	}

	void App::draw_frame_2d(
		const gpu::GraphicsPipeline& line_pipeline,
		const gpu::CommandBuffer& command_buffer,
		const gpu::RenderPass& render_pass
	) noexcept
	{
		const auto& io = ImGui::GetIO();
		const float aspect_ratio = io.DisplaySize.x / io.DisplaySize.y;
		const auto vp_matrix = current_camera_2d.get_matrix(aspect_ratio);

		command_buffer.push_uniform_to_vertex(0, util::as_bytes(vp_matrix));

		render_pass.bind_pipeline(line_pipeline);

		if (temp_buffer.get_buffer().has_value())
		{
			render_pass.bind_vertex_buffers(
				0,
				SDL_GPUBufferBinding{.buffer = temp_buffer.get_buffer().value(), .offset = 0}
			);
			render_pass.draw(temp_buffer_vertex_count, 0, 1, 0);
		}

		if (persistent_vertex_buffer.get_buffer() && persistent_indirect_buffer.get_buffer())
		{
			render_pass.bind_vertex_buffers(
				0,
				SDL_GPUBufferBinding{.buffer = *persistent_vertex_buffer.get_buffer(), .offset = 0}
			);
			render_pass
				.draw_indirect(*persistent_indirect_buffer.get_buffer(), persistent_buffer_indirect_count, 0);
		}
	}

	void App::update_camera_2d() noexcept
	{
		const auto& io = ImGui::GetIO();

		if (!io.WantCaptureMouse)
		{
			if (io.MouseDown[ImGuiMouseButton_Middle])
				target_camera_2d
					.pan({io.MouseDelta.x, -io.MouseDelta.y}, {io.DisplaySize.x, io.DisplaySize.y});

			if (io.MouseWheel != 0.0f)
				target_camera_2d.zoom(
					io.MouseWheel > 0.0f ? 0.9f : 1.1f,
					{io.MousePos.x, io.MousePos.y},
					{io.DisplaySize.x, io.DisplaySize.y}
				);
		}

		const float lerp_factor = glm::clamp(camera_lerp_speed * io.DeltaTime, 0.0f, 1.0f);
		current_camera_2d = Camera2D::mix(current_camera_2d, target_camera_2d, lerp_factor);
	}

	std::expected<std::variant<App::EditState, App::DrawState>, util::Error> App::handle_state_2d(
		SDL_GPUDevice* device,
		std::variant<EditState, DrawState> old_state,
		BottomBarTab new_tab,
		const glm::mat4& vp_matrix
	) noexcept
	{
		auto new_state = std::move(old_state);

		if (get_current_tab(new_state) != new_tab)
		{
			// Switch state
			switch (new_tab)
			{
			case BottomBarTab::Edit:
				new_state = EditState();
				break;
			case BottomBarTab::DrawLine:
				new_state = DrawState(CurveCreator<primitive::Line>());
				break;
			case BottomBarTab::DrawCircle:
				new_state = DrawState(CurveCreator<primitive::Circle>());
				break;
			case BottomBarTab::DrawBezier:
				new_state = DrawState(CurveCreator<primitive::BezierCurve>());
				break;
			case BottomBarTab::DrawCubicSpline:
				new_state = DrawState(CurveCreator<primitive::CubicSpline>());
				break;
			}
		}

		if (std::holds_alternative<EditState>(new_state))
		{
			auto edit_result = handle_edit_2d(device, std::get<EditState>(new_state), vp_matrix);
			if (!edit_result.has_value()) return edit_result.error().forward("Handle edit state failed");
			new_state = edit_result.value();
		}
		else
		{
			auto draw_result = handle_draw_2d(device, std::get<DrawState>(new_state), vp_matrix);
			if (!draw_result.has_value()) return draw_result.error().forward("Handle draw state failed");

			if (draw_result->has_value())
				new_state = DrawState(std::move(draw_result->value()));
			else
				new_state = EditState();
		}

		return new_state;
	}

	std::expected<App::EditState, util::Error> App::handle_edit_2d(
		SDL_GPUDevice* device,
		EditState old_state,
		const glm::mat4& vp_matrix
	) noexcept
	{
		static constexpr float MARGIN = 15.0f;

		const auto& io = ImGui::GetIO();
		auto state = old_state;

		bool update_needed = false;

		const auto entry_ui =
			[&update_needed, &vp_matrix, &old_state, &state]<primitive::PrimitiveType T>(
				std::format_string<long> format,
				std::vector<PrimitiveEntry<T>>& entries
			) {
				std::optional<size_t> to_erase;

				for (auto [idx, entry] : std::views::enumerate(entries))
				{
					const bool selected = old_state.has_value()
						&& std::holds_alternative<PrimitiveIndex<T>>(*old_state)
						&& (std::cmp_equal(std::get<PrimitiveIndex<T>>(*old_state).index, idx));

					auto label = std::format(format, idx + 1);
					if (selected) label = std::format(">> {}", label);

					if (ImGui::TreeNode(label.c_str()))
					{
						glm::vec4 color_vec4 = glm::vec4(entry.get_primitive().color) / 255.0f;
						ImGui::ColorEdit4(
							"颜色",
							&color_vec4.x,
							ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_Uint8
						);
						entry.get_primitive().color = glm::u8vec4(color_vec4 * 255.0f);

						if (selected)
						{
							ImGui::SameLine();
							if (ImGui::Button("完成 \U000f012c")) state = std::nullopt;
						}
						else
						{
							ImGui::SameLine();
							if (ImGui::Button("编辑 \uf044")) state = PrimitiveIndex<T>{.index = size_t(idx)};
							ImGui::SameLine();
							if (ImGui::Button("删除 \U000f05e8") && !to_erase.has_value())
							{
								to_erase = idx;
								if (selected) state = std::nullopt;
							}
						}

						ImGui::TreePop();
					}

					if (selected) entry.get_primitive().edit(vp_matrix);
					if (entry.update()) update_needed = true;
				}

				if (to_erase.has_value())
				{
					entries.erase(entries.begin() + *to_erase);
					update_needed = true;
				}
			};

		ImGui::SetNextWindowPos({io.DisplaySize.x - MARGIN, MARGIN}, ImGuiCond_Always, {1.0f, 0.0f});
		ImGui::SetNextWindowSizeConstraints({400.0f, 200.0}, {FLT_MAX, io.DisplaySize.y - 2 * MARGIN});

		if (ImGui::Begin("曲线列表", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (lines.empty() && circles.empty() && beziers.empty() && splines.empty())
				ImGui::Text("暂无曲线，在下方工具栏选择画线工具以添加曲线。");
			else
			{
				entry_ui("线段 #{0}##Line{0}", lines);
				entry_ui("圆 #{0}##Circle{0}", circles);
				entry_ui("贝塞尔曲线 #{0}##Bezier{0}", beziers);
				entry_ui("三次样条曲线 #{0}##Spline{0}", splines);
			}
		}
		ImGui::End();

		if (update_needed)
			if (const auto rebuild_result = rebuild_persistent_buffer_2d(device); !rebuild_result)
				return rebuild_result.error().forward("Rebuild persistent buffer failed");

		return state;
	}

	std::expected<std::optional<App::DrawState>, util::Error> App::handle_draw_2d(
		SDL_GPUDevice* device,
		DrawState draw_state,
		const glm::mat4& vp_matrix
	) noexcept
	{
		return std::visit(
			[this, &vp_matrix, device]<typename T>(CurveCreator<T> creator)
				-> std::expected<DrawState, util::Error> {
				const auto result = creator.update(vp_matrix, curve_color);

				if (std::holds_alternative<CurveInterrupt>(result))
				{
					if (const auto reset_result = reset_temp_buffer_2d(device); !reset_result.has_value())
						return reset_result.error().forward("Reset temp buffer failed");

					return DrawState(CurveCreator<T>());
				}

				if (std::holds_alternative<CurveContinue>(result))
				{
					const auto curve = creator.get_curve_with_mouse(vp_matrix, curve_color);
					if (curve.has_value())
					{
						curve->draw_ui(vp_matrix);

						const std::vector<LineVertex> vertex_list = curve->gen_vertices();
						if (const auto update_result = update_temp_buffer_2d(device, vertex_list);
							!update_result.has_value())
							return update_result.error().forward("Update temp buffer failed");
					}

					return DrawState(std::move(creator));
				}

				if constexpr (std::same_as<T, primitive::Line>)
				{
					auto curve = std::move(std::get<primitive::Line>(result));
					lines.push_back(PrimitiveEntry<primitive::Line>(std::move(curve)));
				}
				if constexpr (std::same_as<T, primitive::Circle>)
				{
					auto curve = std::move(std::get<primitive::Circle>(result));
					circles.push_back(PrimitiveEntry<primitive::Circle>(std::move(curve)));
				}
				if constexpr (std::same_as<T, primitive::BezierCurve>)
				{
					auto curve = std::move(std::get<primitive::BezierCurve>(result));
					beziers.push_back(PrimitiveEntry<primitive::BezierCurve>(std::move(curve)));
				}
				if constexpr (std::same_as<T, primitive::CubicSpline>)
				{
					auto curve = std::move(std::get<primitive::CubicSpline>(result));
					splines.push_back(PrimitiveEntry<primitive::CubicSpline>(std::move(curve)));
				}

				if (const auto rebuild_result = rebuild_persistent_buffer_2d(device); !rebuild_result)
					return rebuild_result.error().forward("Rebuild persistent buffer failed");

				if (const auto reset_result = reset_temp_buffer_2d(device); !reset_result.has_value())
					return reset_result.error().forward("Reset temp buffer failed");

				return DrawState(CurveCreator<T>());
			},
			std::move(draw_state)
		);
	}

	App::BottomBarTab App::get_current_tab(const std::variant<EditState, DrawState>& state) const noexcept
	{
		if (std::holds_alternative<EditState>(state)) return BottomBarTab::Edit;

		const auto& draw_state = std::get<DrawState>(state);

		return std::visit(
			[](const auto& state) { return draw_tab<typename std::decay_t<decltype(state)>::PrimitiveType>; },
			draw_state
		);
	}

	App::BottomBarTab App::bottom_ui_2d(BottomBarTab old_tab) noexcept
	{
		auto tab = old_tab;

		capsule::window("BottomBar", capsule::Position::BottomCenter, [&tab, old_tab] {
			for (const auto current_tab : {
					 BottomBarTab::Edit,
					 BottomBarTab::DrawLine,
					 BottomBarTab::DrawCircle,
					 BottomBarTab::DrawBezier,
					 BottomBarTab::DrawCubicSpline,
				 })
			{
				const auto [icon, tooltip] = get_bottom_bar_tab_icon(current_tab);

				if (old_tab == current_tab)
				{
					ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 255, 255, 255));
					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
				}

				if (capsule::button(icon)) tab = current_tab;
				ImGui::SetItemTooltip("%s", tooltip);

				if (old_tab == current_tab)
				{
					ImGui::PopStyleVar();
					ImGui::PopStyleColor();
				}

				if (current_tab == BottomBarTab::Edit) capsule::vertical_separator();
			}
		});

		if (tab != BottomBarTab::Edit)
			capsule::window(
				"ColorPicker",
				capsule::Position::BottomCenter,
				[this] {
					glm::vec4 color_vec4 = glm::vec4(curve_color) / 255.0f;
					ImGui::ColorEdit4("##CurveColor", &color_vec4.x, ImGuiColorEditFlags_Uint8);
					curve_color = glm::u8vec4(color_vec4 * 255.0f);
				},
				{0, -1},
				true
			);

		capsule::window("ModeChange", capsule::Position::BottomLeft, [this] {
			if (capsule::button("\U000f07fd")) mode = Mode::Solid3D;
		});

		capsule::window(
			"Hints",
			capsule::Position::BottomRight,
			[] {
				ImGui::SeparatorText("操作提示");
				ImGui::BulletText("按住鼠标中键拖动视角");
				ImGui::BulletText("按下左键绘制曲线，若没有反应则需要多按一次");
				ImGui::BulletText("按Esc取消，按Enter完成，右键撤销上一个控制点");
				ImGui::BulletText("左下角切换3D模式");
				ImGui::SeparatorText("23336160 刘信杰 作业3");
			},
			{0, 0},
			true
		);

		return tab;
	}

	void App::bottom_ui_3d() noexcept
	{
		capsule::window("Controls", capsule::Position::TopRight, [this] {
			for (const auto [i, j] :
				 std::views::cartesian_product(std::views::iota(0, 4), std::views::iota(0, 4)))
			{
				const auto label = std::format("##P{},{}", i, j);

				ImGuiKnobs::Knob(
					label.c_str(),
					&bezier_3d_points[i][j],
					-1.0f,
					1.0f,
					0.03f,
					"%.2f",
					ImGuiKnobVariant_Tick,
					60,
					ImGuiKnobFlags_NoInput | ImGuiKnobFlags_AlwaysClamp | ImGuiKnobFlags_NoTitle
				);

				if (j < 3) ImGui::SameLine();
			}
		});

		capsule::window("ModeChange", capsule::Position::BottomLeft, [this] {
			if (capsule::button("\U000f1a1c")) mode = Mode::Line2D;

			const auto line_mode_icon = "\U000f02c1";
			const auto solid_mode_icon = "\U000f0536";

			capsule::vertical_separator();

			if (mode == Mode::Line3D)
				if (capsule::button(line_mode_icon)) mode = Mode::Solid3D;
			if (mode == Mode::Solid3D)
				if (capsule::button(solid_mode_icon)) mode = Mode::Line3D;
		});

		capsule::window(
			"Hints",
			capsule::Position::BottomRight,
			[] {
				ImGui::SeparatorText("操作提示");
				ImGui::BulletText("按住鼠标中键拖动视角");
				ImGui::BulletText("滚动鼠标滚轮缩放视角");
				ImGui::BulletText("右上角控制点调整贝塞尔曲面形状");
				ImGui::BulletText("左下角切换2D模式和3D显示模式");
				ImGui::SeparatorText("23336160 刘信杰 作业3");
			},
			{0, 0},
			true
		);
	}

	void App::performance_overlay() const noexcept
	{
		static constexpr float MARGIN = 15.0f;

		const auto& io = ImGui::GetIO();

		const auto formatted_info =
			std::format("FPS: {:.1f} ({:.1f} ms)", io.Framerate, 1000.0f / io.Framerate);

		auto drawlist = ImGui::GetBackgroundDrawList();

		drawlist->AddText({MARGIN, MARGIN}, IM_COL32(255, 255, 255, 255), formatted_info.c_str());
	}
}