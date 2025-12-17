#pragma once

#include "graphics/camera/projection/perspective.hpp"
#include "graphics/camera/view/orbit.hpp"

#include <glm/glm.hpp>

namespace logic
{
	class Camera
	{
	  public:

		struct Matrices
		{
			glm::mat4 view_matrix;
			glm::mat4 proj_matrix;
			glm::mat4 prev_camera_matrix;
			glm::vec3 eye_position;
		};

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
		Matrices get_matrices() noexcept;

	  private:

		using Perspective = graphics::camera::projection::Perspective;
		using Orbit = graphics::camera::view::Orbit;
		using Pan_controller = graphics::camera::view::Orbit::Pan_controller;
		using Rotate_controller = graphics::camera::view::Orbit::Rotate_controller;

		Perspective camera_projection = Perspective(glm::radians(45.0), 0.5, std::nullopt);
		Orbit camera_orbit = Orbit(
			3,
			glm::radians(90.0),
			glm::radians(20.0),
			glm::vec3(0.0, 0.5, 0.0),
			glm::vec3(0.0, 1.0, 0.0)
		);
		Pan_controller pan_controller = Orbit::Pan_controller{0.5f};
		Rotate_controller rotate_controller = Rotate_controller{
			.azimuth_per_width = glm::radians(360.0f),
			.pitch_per_height = glm::radians(180.0f),
		};

		std::optional<glm::dmat4> prev_frame_camera_matrix;
	};
}