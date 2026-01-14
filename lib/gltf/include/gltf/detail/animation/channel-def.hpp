#pragma once

#include "gltf/node.hpp"

#include <span>

namespace gltf::detail::animation
{
	// General interface for animation channels
	class Channel
	{
	  public:

		virtual ~Channel() = default;

		///
		/// @brief Apply the animation channel at the given time to node transform overrides
		/// @note This doesn't check for out-of-bounds access on overrides
		/// @param overrides Node transform overrides
		/// @param time Absolute timestamp
		///
		virtual void apply(std::span<Node::TransformOverride> overrides, float time) const noexcept = 0;
	};
}