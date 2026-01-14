#include "gltf/detail/animation/channels.hpp"

namespace gltf::detail::animation
{
	void TranslationChannel::apply(std::span<Node::TransformOverride> overrides, float time) const noexcept
	{
		overrides[target_node].translation = sampler[time];
	}

	void RotationChannel::apply(std::span<Node::TransformOverride> overrides, float time) const noexcept
	{
		overrides[target_node].rotation = sampler[time];
	}

	void ScaleChannel::apply(std::span<Node::TransformOverride> overrides, float time) const noexcept
	{
		overrides[target_node].scale = sampler[time];
	}
}