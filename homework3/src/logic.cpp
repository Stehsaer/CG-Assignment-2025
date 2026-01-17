#include "logic.hpp"
#include "capsule-ui.hpp"
#include "geometry/primitive.hpp"
#include "geometry/vertex.hpp"
#include "util/as-byte.hpp"

#include <SDL3/SDL_gpu.h>
#include <format>
#include <imgui.h>
#include <set>
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
		update_camera();
		const auto vp_matrix =
			current_camera.get_matrix(ImGui::GetIO().DisplaySize.x / ImGui::GetIO().DisplaySize.y);

		const auto new_tab = bottom_bar_ui(get_current_tab(draw_edit_state));
		auto new_state = handle_state(device, std::move(draw_edit_state), new_tab, vp_matrix);
		if (!new_state.has_value()) return new_state.error().forward("Handle app state failed");
		draw_edit_state = std::move(new_state.value());

		performance_overlay();

		return {};
	}

	std::expected<void, util::Error> App::update_temp_buffer(
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

	std::expected<void, util::Error> App::reset_temp_buffer(SDL_GPUDevice* device [[maybe_unused]]) noexcept
	{
		temp_buffer_vertex_count = 0;
		return {};
	}

	std::expected<void, util::Error> App::rebuild_persistent_buffer(SDL_GPUDevice* device) noexcept
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
		temp_buffer.copy_to_gpu(copy_pass);
		persistent_vertex_buffer.copy_to_gpu(copy_pass);
		persistent_indirect_buffer.copy_to_gpu(copy_pass);
	}

	void App::draw_frame(
		const gpu::GraphicsPipeline& line_pipeline,
		const gpu::CommandBuffer& command_buffer,
		const gpu::RenderPass& render_pass
	) noexcept
	{
		const auto& io = ImGui::GetIO();
		const float aspect_ratio = io.DisplaySize.x / io.DisplaySize.y;
		const auto vp_matrix = current_camera.get_matrix(aspect_ratio);

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

	void App::update_camera() noexcept
	{
		const auto& io = ImGui::GetIO();

		if (!io.WantCaptureMouse)
		{
			if (io.MouseDown[ImGuiMouseButton_Middle])
				target_camera.pan({io.MouseDelta.x, -io.MouseDelta.y}, {io.DisplaySize.x, io.DisplaySize.y});

			if (io.MouseWheel != 0.0f)
				target_camera.zoom(
					io.MouseWheel > 0.0f ? 0.9f : 1.1f,
					{io.MousePos.x, io.MousePos.y},
					{io.DisplaySize.x, io.DisplaySize.y}
				);
		}

		const float lerp_factor = glm::clamp(camera_lerp_speed * io.DeltaTime, 0.0f, 1.0f);
		current_camera = Camera2D::mix(current_camera, target_camera, lerp_factor);
	}

	std::expected<std::variant<App::EditState, App::DrawState>, util::Error> App::handle_state(
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
			auto edit_result = handle_edit(device, std::get<EditState>(new_state), vp_matrix);
			if (!edit_result.has_value()) return edit_result.error().forward("Handle edit state failed");
			new_state = edit_result.value();
		}
		else
		{
			auto draw_result = handle_draw(device, std::get<DrawState>(new_state), vp_matrix);
			if (!draw_result.has_value()) return draw_result.error().forward("Handle draw state failed");

			if (draw_result->has_value())
				new_state = DrawState(std::move(draw_result->value()));
			else
				new_state = EditState();
		}

		return new_state;
	}

	std::expected<App::EditState, util::Error> App::handle_edit(
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
			if (!lines.empty() || !circles.empty() || !beziers.empty() || !splines.empty())
				ImGui::Text("暂无曲线，在下方工具栏选择画线工具以添加曲线。");

			entry_ui("线段 #{0}##Line{0}", lines);
			entry_ui("圆 #{0}##Circle{0}", circles);
			entry_ui("贝塞尔曲线 #{0}##Bezier{0}", beziers);
			entry_ui("三次样条曲线 #{0}##Spline{0}", splines);
		}
		ImGui::End();

		if (update_needed)
			if (const auto rebuild_result = rebuild_persistent_buffer(device); !rebuild_result)
				return rebuild_result.error().forward("Rebuild persistent buffer failed");

		return state;
	}

	std::expected<std::optional<App::DrawState>, util::Error> App::handle_draw(
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
					if (const auto reset_result = reset_temp_buffer(device); !reset_result.has_value())
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
						if (const auto update_result = update_temp_buffer(device, vertex_list);
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

				if (const auto rebuild_result = rebuild_persistent_buffer(device); !rebuild_result)
					return rebuild_result.error().forward("Rebuild persistent buffer failed");

				if (const auto reset_result = reset_temp_buffer(device); !reset_result.has_value())
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

	App::BottomBarTab App::bottom_bar_ui(BottomBarTab old_tab) noexcept
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

				if (old_tab == current_tab)
				{
					ImGui::PopStyleVar();
					ImGui::PopStyleColor();
				}
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

		return tab;
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