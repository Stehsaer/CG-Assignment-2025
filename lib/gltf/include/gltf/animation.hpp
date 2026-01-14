#pragma once

#include "detail/animation/channel-def.hpp"
#include "gltf/node.hpp"
#include "util/error.hpp"

#include <expected>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

namespace gltf
{
	struct AnimationKey
	{
		std::variant<uint32_t, std::string> animation;
		float time;
	};

	class Animation
	{
	  public:

		///
		/// @brief Create an animation from a tinygltf animation
		///
		/// @param model TinyGLTF model
		/// @param animation TinyGLTF animation
		/// @return Created animation or error
		///
		static std::expected<Animation, util::Error> from_tinygltf(
			const tinygltf::Model& model,
			const tinygltf::Animation& animation
		) noexcept;

		///
		/// @brief Apply the animation at the given time to node transform overrides
		///
		/// @param overrides Node transform overrides
		/// @param time Absolute timestamp
		///
		void apply(std::span<Node::TransformOverride> overrides, float time) const noexcept;

		// Name of the animation, can be none
		std::optional<std::string> name;

	  private:

		std::vector<std::unique_ptr<detail::animation::Channel>> channels;

		Animation(
			std::optional<std::string> name,
			std::vector<std::unique_ptr<detail::animation::Channel>> channels
		) :
			name(std::move(name)),
			channels(std::move(channels))
		{}

	  public:

		Animation(const Animation&) = delete;
		Animation(Animation&&) = default;
		Animation& operator=(const Animation&) = delete;
		Animation& operator=(Animation&&) = default;
	};
}