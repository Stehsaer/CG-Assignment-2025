#pragma once

#include "backend/sdl.hpp"
#include "graphics/camera/projection/perspective.hpp"
#include "graphics/camera/view/flying.hpp"
#include "graphics/camera/view/orbit.hpp"
#include "render/param.hpp"

#include <glm/glm.hpp>

namespace logic
{
	class Camera
	{
	  public:

		///
		/// @brief Update and get camera matrices
		///
		/// @param camera Target camera view
		/// @return Current camera matrices
		///
		render::Camera_matrices update(const graphics::camera::view::Flying& camera) noexcept;

	  private:

		graphics::camera::projection::Perspective camera_projection =
			{.fov_y = glm::radians(45.0f), .near_plane = 0.15, .far_plane = std::nullopt};

		std::optional<graphics::camera::view::Flying> camera;
		std::optional<glm::dmat4> prev_frame_camera_matrix;

		static constexpr float mix_factor = 16.0f;
	};

	class Free_camera
	{
	  public:

		///
		/// @brief Update free camera based on user input, and returns current camera view
		///
		/// @param context SDL backend context
		/// @param free_cam Whether to enable free camera mode (false = fixed 1.5m height)
		/// @return Current camera view
		///
		graphics::camera::view::Flying update(const backend::SDL_context& context, bool free_cam) noexcept;

	  private:

		graphics::camera::view::Flying target_camera = {
			.position = glm::dvec3(0.0, 1.5, 0.0),
			.angles = {.azimuth = glm::radians(90.0f), .pitch = glm::radians(-20.0f)},
			.up = glm::dvec3(0.0, 1.0, 0.0)
		};

		inline static const float azimuth_per_width = glm::radians(180.0f);
		inline static const float pitch_per_height = glm::radians(90.0f);
	};
}