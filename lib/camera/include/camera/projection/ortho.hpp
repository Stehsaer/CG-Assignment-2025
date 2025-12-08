#include "camera/projection.hpp"

#include <glm/vec2.hpp>

namespace camera::projection
{
	///
	/// @brief Orthographic Projection, with view cube's aspect ratio matching the viewport's
	///
	///
	struct Ortho : public Projection
	{
		Ortho(const Ortho&) = default;
		Ortho(Ortho&&) = default;
		Ortho& operator=(const Ortho&) = default;
		Ortho& operator=(Ortho&&) = default;

		///
		/// @brief Create an orthographic projection
		///
		/// @param viewport_height Viewport height of the view cube
		/// @param near_plane Distance to near plane
		/// @param far_plane Distance to far plane
		///
		Ortho(float viewport_height, float near_plane, float far_plane) noexcept;
		virtual ~Ortho() = default;

		glm::dmat4 matrix(float aspect_ratio) noexcept override;

		float viewport_height;
		float near_plane;
		float far_plane;
	};

	///
	/// @brief Fixed-aspect Orthographic Projection
	///
	struct Ortho_fixed : public Projection
	{
		Ortho_fixed(const Ortho_fixed&) = default;
		Ortho_fixed(Ortho_fixed&&) = default;
		Ortho_fixed& operator=(const Ortho_fixed&) = default;
		Ortho_fixed& operator=(Ortho_fixed&&) = default;

		///
		/// @brief Create a fixed-aspect orthographic projection
		///
		/// @param viewport_size Width and height of the view cube
		/// @param near_plane Distance to near plane
		/// @param far_plane Distance to far plane
		///
		Ortho_fixed(glm::vec2 viewport_size, float near_plane, float far_plane) noexcept;
		virtual ~Ortho_fixed() = default;

		glm::dmat4 matrix(float aspect_ratio) noexcept override;

		glm::vec2 viewport_size;
		float near_plane;
		float far_plane;
	};
}
