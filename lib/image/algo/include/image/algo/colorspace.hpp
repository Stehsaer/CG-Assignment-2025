#pragma once

#include <glm/ext/matrix_float3x3.hpp>

namespace image::colorspace
{
	static const glm::mat3 rgb_to_ycbcr_matrix =
		glm::mat3(0.299f, -0.168736f, 0.500f, 0.587f, -0.331264f, -0.460525f, 0.114f, 0.500f, -0.081275f);

	static const glm::mat3 ycbcr_to_rgb_matrix = glm::inverse(rgb_to_ycbcr_matrix);

	///
	/// @brief Converts RGB color to YCbCr color space
	///
	/// @param rgb RGB color, [0, 1] range
	/// @return YCbCr color
	///
	inline glm::vec3 rgb_to_ycbcr(glm::vec3 rgb) noexcept
	{
		return rgb_to_ycbcr_matrix * rgb + glm::vec3(0.0f, 0.5f, 0.5f);
	}

	///
	/// @brief Converts RGB color to YCbCr color space, with Alpha channel preserved
	///
	/// @param rgba RGBA color, [0, 1] range
	/// @return YCbCrA color
	///
	inline glm::vec4 rgba_to_ycbcr_alpha(glm::vec4 rgba) noexcept
	{
		return glm::vec4(rgb_to_ycbcr(glm::vec3(rgba)), rgba.a);
	}

	///
	/// @brief Converts YCbCr color to RGB color space
	///
	/// @param ycbcr YCbCr color
	/// @return RGB color, [0, 1] range
	///
	inline glm::vec3 ycbcr_to_rgb(glm::vec3 ycbcr) noexcept
	{
		return ycbcr_to_rgb_matrix * (ycbcr - glm::vec3(0.0f, 0.5f, 0.5f));
	}

	///
	/// @brief Converts YCbCr color to RGB color space, with Alpha channel preserved
	///
	/// @param ycbcra YCbCrA color
	/// @return RGBA color, [0, 1] range
	///
	inline glm::vec4 ycbcr_alpha_to_rgba(glm::vec4 ycbcra) noexcept
	{
		return glm::vec4(ycbcr_to_rgb(glm::vec3(ycbcra)), ycbcra.a);
	}
}