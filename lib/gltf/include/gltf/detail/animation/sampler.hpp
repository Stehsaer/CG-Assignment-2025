#pragma once

#include <algorithm>
#include <optional>
#include <ranges>
#include <tiny_gltf.h>
#include <variant>
#include <vector>

#include "gltf/accessor.hpp"
#include "interpolation.hpp"
#include "util/error.hpp"
#include "util/inline.hpp"

namespace gltf::detail::animation
{
	// Interpolation mode for animation samplers
	enum class Interpolation
	{
		Linear,
		Step,
		Cubic
	};

	///
	/// @brief Parse interpolation mode from string
	///
	/// @param str Input string
	/// @return Interpolation mode, or nullopt if unknown
	///
	std::optional<Interpolation> parse_interpolation(const std::string& str) noexcept;

	// Animation sampler for a specific type T
	template <typename T>
	class Sampler
	{
		// Interpolation method
		Interpolation interpolation;

		// Keyframes stored as (time, value) pairs
		std::variant<
			std::vector<std::pair<float, T>>,
			std::vector<std::pair<float, detail::animation::CubicKeyFrame<T>>>
		>
			keyframes;

		Sampler(std::vector<std::pair<float, T>> keyframes, Interpolation interpolation) :
			interpolation(interpolation),
			keyframes(std::move(keyframes))
		{
			assert(interpolation == Interpolation::Linear || interpolation == Interpolation::Step);
		}

		Sampler(
			std::vector<std::pair<float, detail::animation::CubicKeyFrame<T>>> keyframes,
			Interpolation interpolation
		) :
			interpolation(interpolation),
			keyframes(std::move(keyframes))
		{
			assert(interpolation == Interpolation::Cubic);
		}

	  public:

		static std::expected<Sampler<T>, util::Error> from_tinygltf(
			const tinygltf::Model& model,
			const tinygltf::AnimationSampler& sampler
		) noexcept;

		T operator[](float time) const noexcept;

		Sampler(const Sampler&) = delete;
		Sampler(Sampler&&) = default;
		Sampler& operator=(const Sampler&) = delete;
		Sampler& operator=(Sampler&&) = default;
	};

	template <typename T>
	FORCE_INLINE inline T Sampler<T>::operator[](float time) const noexcept
	{
		switch (interpolation)
		{
		case Interpolation::Linear:
		case Interpolation::Step:
		{
			using Vec_type = std::vector<std::pair<float, T>>;
			assert((std::holds_alternative<Vec_type>(keyframes)));
			const auto& keyframe_vec = std::get<Vec_type>(keyframes);

			const auto upper = std::ranges::upper_bound(keyframe_vec, time, {}, &std::pair<float, T>::first);
			if (upper == keyframe_vec.begin()) return upper->second;
			if (upper == keyframe_vec.end()) return std::prev(upper)->second;

			const auto lower = std::prev(upper);
			if (interpolation == Interpolation::Step) return lower->second;

			return detail::animation::interpolate_linear(
				lower->second,
				upper->second,
				lower->first,
				upper->first,
				time
			);
		}

		case Interpolation::Cubic:
		{
			using Vec_type = std::vector<std::pair<float, detail::animation::CubicKeyFrame<T>>>;
			assert((std::holds_alternative<Vec_type>(keyframes)));
			const auto& keyframe_vec = std::get<Vec_type>(keyframes);

			const auto upper = std::ranges::upper_bound(
				keyframe_vec,
				time,
				{},
				&std::pair<float, detail::animation::CubicKeyFrame<T>>::first
			);
			if (upper == keyframe_vec.begin()) return upper->second.value;
			if (upper == keyframe_vec.end()) return std::prev(upper)->second.value;

			const auto lower = std::prev(upper);
			return detail::animation::interpolate_cubic_spline(
				lower->second,
				upper->second,
				lower->first,
				upper->first,
				time
			);
		}

		default:
			std::unreachable();
		}
	}

	template <typename T>
	std::expected<Sampler<T>, util::Error> Sampler<T>::from_tinygltf(
		const tinygltf::Model& model,
		const tinygltf::AnimationSampler& sampler
	) noexcept
	{
		/* Validate Indices */

		if (sampler.input < 0 || std::cmp_greater_equal(sampler.input, model.accessors.size()))
			return util::Error("Invalid accessor index for animation sampler input");

		if (sampler.output < 0 || std::cmp_greater_equal(sampler.output, model.accessors.size()))
			return util::Error("Invalid accessor index for animation sampler output");

		/* Get Interpolation Mode */

		auto interpolation_result = parse_interpolation(sampler.interpolation);
		if (!interpolation_result)
			return util::Error(std::format("Unknown interpolation type: {}", sampler.interpolation));
		const auto interpolation = *interpolation_result;

		/* Extract Accessor Data */

		auto timestamps_result = extract_from_accessor<float>(model, model.accessors[sampler.input]);
		if (!timestamps_result) return timestamps_result.error().forward("Extract timestamps failed");
		const auto timestamps = std::move(*timestamps_result);

		auto values_result = extract_from_accessor<T>(model, model.accessors[sampler.output]);
		if (!values_result) return values_result.error().forward("Extract values failed");
		const auto values = std::move(*values_result);

		/* Validate Accessor Data */

		switch (interpolation)
		{
		case Interpolation::Linear:
		case Interpolation::Step:
		{
			if (timestamps.size() == 0) return util::Error("Animation sampler has zero keyframes");
			if (timestamps.size() != values.size())
				return util::Error(
					std::format(
						"Animation sampler timestamps size ({})) does not match values size ({})",
						timestamps.size(),
						values.size()
					)
				);

			auto keyframes =
				std::views::zip_transform(
					[](float timestamp, const T& value) -> std::pair<float, T> {
						return std::make_pair(timestamp, value);
					},
					timestamps,
					values
				)
				| std::ranges::to<std::vector<std::pair<float, T>>>();

			std::ranges::sort(keyframes, {}, &std::pair<float, T>::first);

			return Sampler<T>(std::move(keyframes), interpolation);
		}
		case Interpolation::Cubic:
		{
			if (timestamps.size() < 2)
				return util::Error("Cubic spline animation sampler requires at least two keyframes");
			if (timestamps.size() * 3 != values.size())
				return util::Error(
					std::format(
						"Cubic spline animation sampler timestamps size ({}) does not match values size "
						"({})",
						timestamps.size(),
						values.size()
					)
				);

			std::vector<std::pair<float, detail::animation::CubicKeyFrame<T>>> keyframes;
			keyframes.reserve(timestamps.size());

			for (const auto idx : std::views::iota(0u, timestamps.size()))
			{
				detail::animation::CubicKeyFrame<T> keyframe;
				keyframe.in_tangent = values[idx * 3 + 0];
				keyframe.value = values[idx * 3 + 1];
				keyframe.out_tangent = values[idx * 3 + 2];

				keyframes.emplace_back(timestamps[idx], keyframe);
			}

			std::ranges::sort(keyframes, {}, &std::pair<float, detail::animation::CubicKeyFrame<T>>::first);

			return Sampler<T>(std::move(keyframes), interpolation);
		}
		default:
			std::unreachable();
		}
	}
}