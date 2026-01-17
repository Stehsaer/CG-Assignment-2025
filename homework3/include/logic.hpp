#pragma once

#include "geometry/primitive.hpp"
#include "geometry/vertex.hpp"
#include "gpu/command-buffer.hpp"
#include "gpu/copy-pass.hpp"
#include "gpu/graphics-pipeline.hpp"
#include "gpu/render-pass.hpp"
#include "graphics/camera/projection/perspective.hpp"
#include "graphics/camera/view/orbit.hpp"
#include "logic/camera-2d.hpp"
#include "logic/curve-creator.hpp"
#include "logic/dynamic-gpu-buf.hpp"
#include "logic/vertex-cache.hpp"
#include "pipeline/surface.hpp"

#include <optional>

namespace logic
{
	enum class PrimitiveMode
	{
		Line,
		Circle,
		Bezier,
		CubicSpline
	};

	template <primitive::PrimitiveType T>
	class PrimitiveEntry
	{
		T primitive;
		logic::VertexCache<T> vertex_cache;

	  public:

		PrimitiveEntry(T primitive) :
			primitive(std::move(primitive))
		{
			vertex_cache.update(this->primitive);
		}

		std::span<const LineVertex> get() const noexcept { return vertex_cache.get(); }

		bool update() { return vertex_cache.update(this->primitive); }

		T& get_primitive(this auto& self) noexcept { return self.primitive; }
	};

	template <primitive::PrimitiveType T>
	struct PrimitiveIndex
	{
		size_t index;
	};

	class App
	{
	  public:

		std::expected<void, util::Error> imgui_frame(SDL_GPUDevice* device) noexcept;

		void upload_frame(const gpu::CopyPass& copy_pass) noexcept;

		void draw_frame(
			const gpu::GraphicsPipeline& line_pipeline,
			const pipeline::Surface& surface_pipeline,
			const gpu::CommandBuffer& command_buffer,
			const gpu::RenderPass& render_pass
		) noexcept;

	  private:

		void upload_frame_2d(const gpu::CopyPass& copy_pass) noexcept;

		void draw_frame_2d(
			const gpu::GraphicsPipeline& line_pipeline,
			const gpu::CommandBuffer& command_buffer,
			const gpu::RenderPass& render_pass
		) noexcept;

		enum class BottomBarTab
		{
			Edit,
			DrawLine,
			DrawCircle,
			DrawBezier,
			DrawCubicSpline,
		};

		enum class Mode
		{
			Line2D,
			Solid3D,
			Line3D
		};

		Mode mode = Mode::Line2D;

		template <primitive::PrimitiveType T>
		static constexpr BottomBarTab draw_tab = BottomBarTab::Edit;

		using DrawState = std::variant<
			CurveCreator<primitive::Line>,
			CurveCreator<primitive::Circle>,
			CurveCreator<primitive::BezierCurve>,
			CurveCreator<primitive::CubicSpline>
		>;

		using EditState = std::optional<std::variant<
			PrimitiveIndex<primitive::Line>,
			PrimitiveIndex<primitive::Circle>,
			PrimitiveIndex<primitive::BezierCurve>,
			PrimitiveIndex<primitive::CubicSpline>
		>>;

		/* Camera */

		Camera2D target_camera_2d;
		Camera2D current_camera_2d;

		graphics::camera::projection::Perspective
			camera_3d_projection{.fov_y = glm::radians(30.0f), .near_plane = 0.03f, .far_plane = 100.0f};
		graphics::camera::view::Orbit camera_3d_view{
			.distance = 4.0f,
			.angles = {.azimuth = glm::radians(45.0f), .pitch = glm::radians(45.0f)},
			.center = {0.0f, 0.0f, 0.0f},
			.up = {0.0f, 1.0f, 0.0f}
		};

		static constexpr float camera_lerp_speed = 10.0f;

		void update_camera_2d() noexcept;

		/* CPU Storage */

		std::vector<PrimitiveEntry<primitive::Line>> lines;
		std::vector<PrimitiveEntry<primitive::Circle>> circles;
		std::vector<PrimitiveEntry<primitive::BezierCurve>> beziers;
		std::vector<PrimitiveEntry<primitive::CubicSpline>> splines;

		/* GPU Storage */

		DynamicGPUBuffer temp_buffer{true, false};       // Vertex
		DynamicGPUBuffer persistent_vertex_buffer{true, false};    // Vertex
		DynamicGPUBuffer persistent_indirect_buffer{false, true};  // Indirect

		size_t temp_buffer_vertex_count = 0;
		size_t persistent_buffer_indirect_count = 0;
		size_t persistent_buffer_vertex_count = 0;

		// Rebuilds the persistent buffer from CPU storage, updates `persistent_buffer`
		std::expected<void, util::Error> rebuild_persistent_buffer_2d(SDL_GPUDevice* device) noexcept;

		// Updates the temporary buffer with given vertices, updates `temp_buffer`
		std::expected<void, util::Error> update_temp_buffer_2d(
			SDL_GPUDevice* device,
			std::span<const LineVertex> vertices
		) noexcept;

		std::expected<void, util::Error> reset_temp_buffer_2d(SDL_GPUDevice* device) noexcept;

		/* Draw & Edit States */

		std::variant<EditState, DrawState> draw_edit_state = EditState(std::nullopt);
		glm::u8vec4 curve_color = {255, 255, 255, 255};
		glm::mat4 bezier_3d_points = glm::mat4(0.0f);

		std::expected<std::variant<App::EditState, App::DrawState>, util::Error> handle_state_2d(
			SDL_GPUDevice* device,
			std::variant<EditState, DrawState> old_state,
			BottomBarTab new_tab,
			const glm::mat4& vp_matrix
		) noexcept;

		std::expected<EditState, util::Error> handle_edit_2d(
			SDL_GPUDevice* device,
			EditState old_state,
			const glm::mat4& vp_matrix
		) noexcept;

		std::expected<std::optional<App::DrawState>, util::Error> handle_draw_2d(
			SDL_GPUDevice* device,
			DrawState draw_state,
			const glm::mat4& vp_matrix
		) noexcept;

		BottomBarTab get_current_tab(const std::variant<EditState, DrawState>& state) const noexcept;

		/* UI */

		static std::pair<const char*, const char*> get_bottom_bar_tab_icon(BottomBarTab tab) noexcept;

		BottomBarTab bottom_ui_2d(BottomBarTab old_tab) noexcept;
		void bottom_ui_3d() noexcept;

		void performance_overlay() const noexcept;
	};

	template <>
	constexpr App::BottomBarTab App::draw_tab<primitive::Line> = App::BottomBarTab::DrawLine;
	template <>
	constexpr App::BottomBarTab App::draw_tab<primitive::Circle> = App::BottomBarTab::DrawCircle;
	template <>
	constexpr App::BottomBarTab App::draw_tab<primitive::BezierCurve> = App::BottomBarTab::DrawBezier;
	template <>
	constexpr App::BottomBarTab App::draw_tab<primitive::CubicSpline> = App::BottomBarTab::DrawCubicSpline;
}