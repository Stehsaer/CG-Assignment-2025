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
		/// @brief Update camera control based on user input.
		///
		///
		void update(const backend::SDL_context& context) noexcept;

		///
		/// @brief Get camera matrices and eye position. Must be called exactly once per frame.
		///
		/// @return Camera matrices
		///
		render::Camera_matrices get_matrices() noexcept;

	  private:

		using Perspective = graphics::camera::projection::Perspective;
		using Orbit = graphics::camera::view::Orbit;
		using Flying = graphics::camera::view::Flying;

		Perspective camera_projection =
			{.fov_y = glm::radians(45.0f), .near_plane = 0.5, .far_plane = std::nullopt};

		Flying target_camera = {
			.position = glm::dvec3(0.0, 1.5, 0.0),
			.angles = {.azimuth = glm::radians(90.0f), .pitch = glm::radians(-20.0f)},
			.up = glm::dvec3(0.0, 1.0, 0.0)
		};
		Flying lerp_camera = target_camera;

		const float azimuth_per_width = glm::radians(180.0f);
		const float pitch_per_height = glm::radians(90.0f);
		const float mix_factor = 16.0f;

		std::optional<glm::dmat4> prev_frame_camera_matrix;
	};
}