#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace gltf::detail::animation
{
	///
	/// @brief Linear interpolation between two values
	/// @note The input must satisfy `at <= t <= bt`
	/// @tparam T Type of value
	/// @param a Keyframe A
	/// @param b Keyframe B
	/// @param at Timestamp of A
	/// @param bt Timestamp of B
	/// @param t Absolute timestamp
	/// @return Interpolated value
	///
	template <typename T>
	T interpolate_linear(const T& a, const T& b, float at, float bt, float t) noexcept
	{
		assert(at <= t && t <= bt);
		return glm::mix(a, b, (t - at) / (bt - at));
	}

	template <>
	glm::quat interpolate_linear(
		const glm::quat& a,
		const glm::quat& b,
		float at,
		float bt,
		float t
	) noexcept;

	// Cubic Spline Keyframe
	template <typename T>
	struct CubicKeyFrame
	{
		T in_tangent;
		T value;
		T out_tangent;
	};

	///
	/// @brief Interpolates between two cubic spline keyframes
	///
	/// @tparam T Type of value
	/// @param a Keyframe A
	/// @param b Keyframe B
	/// @param at Absolute timestamp of A
	/// @param bt Absolute timestamp of B
	/// @param t Absolute timestamp
	/// @return Interpolated value
	///
	template <typename T>
	T interpolate_cubic_spline(
		const CubicKeyFrame<T>& a,
		const CubicKeyFrame<T>& b,
		float at,
		float bt,
		float t
	) noexcept
	{
		assert(at <= t && t <= bt);

		const float td = bt - at;        // Time duration
		const float tn = (t - at) / td;  // Normalized segment time

		const float t2 = tn * tn;  // tn^2
		const float t3 = t2 * tn;  // tn^3

		const T val = (2 * t3 - 3 * t2 + 1) * a.value
			+ (t3 - 2 * t2 + tn) * td * a.out_tangent
			+ (-2 * t3 + 3 * t2) * b.value
			+ (t3 - t2) * td * b.in_tangent;

		if constexpr (std::same_as<T, glm::quat>)
			return glm::normalize(val);
		else
			return val;
	}
}