#pragma once

#include <glm/glm.hpp>

namespace graphics::camera::view
{
	///
	/// @brief Orbiting View
	///
	///
	struct Orbit
	{
		float distance;
		float azimuth;
		float pitch;
		glm::vec3 center;
		glm::vec3 up;

		glm::dmat4 matrix() const noexcept;
		glm::vec3 eye_position() const noexcept;

		///
		/// @brief Panning controller for Orbit views
		///
		///
		struct Pan_controller
		{
			///
			/// @brief Conversion factor from half-viewport height to view space units
			///
			float conversion_factor;

			///
			/// @brief Pans the orbit view
			///
			/// @param orbit Orbit view to pan
			/// @param screen_size Screen size in pixels
			/// @param pixel_delta Pixel delta to pan
			///
			void pan(Orbit& orbit, glm::vec2 screen_size, glm::vec2 pixel_delta) const noexcept;
		};

		///
		/// @brief Rotation controller for Orbit views
		///
		///
		struct Rotate_controller
		{
			float azimuth_per_width;  // Radians of azimuth change per screen width
			float pitch_per_height;   // Radians of pitch change per screen height

			///
			/// @brief Rotates the orbit view
			///
			/// @param orbit Orbit view to rotate
			/// @param screen_size Screen size in pixels
			/// @param pixel_delta Pixel delta to rotate
			///
			void rotate(Orbit& orbit, glm::vec2 screen_size, glm::vec2 pixel_delta) const noexcept;
		};
	};
}