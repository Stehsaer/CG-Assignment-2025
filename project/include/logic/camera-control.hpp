#pragma once

#include "graphics/camera/projection/perspective.hpp"
#include "graphics/camera/view/orbit.hpp"
#include "render/param.hpp"

#include <glm/glm.hpp>

namespace logic
{
	class Camera
	{
	  public:

		///
		/// @brief Update camera control based on user input.
		///
		///
		void update() noexcept;

		///
		/// @brief Get camera matrices and eye position. Must be called exactly once per frame.
		///
		/// @return Camera matrices
		///
		render::Camera_matrices get_matrices() noexcept;

	  private:

		using Perspective = graphics::camera::projection::Perspective;
		using Orbit = graphics::camera::view::Orbit;
		using Pan_controller = graphics::camera::view::Orbit::Pan_controller;
		using Rotate_controller = graphics::camera::view::Orbit::Rotate_controller;

		Perspective camera_projection =
			{.fov_y = glm::radians(45.0f), .near_plane = 0.5, .far_plane = std::nullopt};

		Orbit target_camera_orbit = {
			.distance = 3,
			.azimuth = glm::radians(90.0f),
			.pitch = glm::radians(20.0f),
			.center = glm::vec3(0.0, 0.5, 0.0),
			.up = glm::vec3(0.0, 1.0, 0.0)
		};
		Orbit lerped_camera_orbit = target_camera_orbit;

		Pan_controller pan_controller = {.conversion_factor = 0.5f};
		Rotate_controller rotate_controller = Rotate_controller{
			.azimuth_per_width = glm::radians(360.0f),
			.pitch_per_height = glm::radians(180.0f),
		};

		const float mix_factor = 16.0f;

		std::optional<glm::dmat4> prev_frame_camera_matrix;
	};
}