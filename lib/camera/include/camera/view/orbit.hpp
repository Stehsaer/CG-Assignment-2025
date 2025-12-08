#pragma once

#include "camera/view.hpp"

#include <glm/vec3.hpp>

namespace camera::view
{
	///
	/// @brief Orbiting View
	///
	///
	struct Orbit : public View
	{
		Orbit(const Orbit&) = default;
		Orbit(Orbit&&) = default;
		Orbit& operator=(const Orbit&) = default;
		Orbit& operator=(Orbit&&) = default;

		///
		/// @brief Create an orbiting view
		///
		/// @param distance Distance from eye to target
		/// @param azimuth (Horizonal) Azimuth angle from target to eye
		/// @param pitch (Vertical) Pitch angle from target to eye
		/// @param center Target position
		/// @param up Up direction
		///
		Orbit(float distance, float azimuth, float pitch, glm::vec3 center, glm::vec3 up) noexcept;

		virtual ~Orbit() = default;

		glm::dmat4 matrix() noexcept override;

		float distance;
		float azimuth;
		float pitch;
		glm::vec3 center;
		glm::vec3 up;

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