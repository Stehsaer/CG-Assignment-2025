#pragma once

#include "channel-def.hpp"
#include "sampler.hpp"

namespace gltf::detail::animation
{
	class TranslationChannel : public Channel
	{
		uint32_t target_node;
		Sampler<glm::vec3> sampler;

	  public:

		// Note: No out-of-bound check for `target_node`
		TranslationChannel(uint32_t target_node, Sampler<glm::vec3> sampler) :
			target_node(target_node),
			sampler(std::move(sampler))
		{}

		void apply(std::span<Node::TransformOverride> overrides, float time) const noexcept override;
	};

	class RotationChannel : public Channel
	{
		uint32_t target_node;
		Sampler<glm::quat> sampler;

	  public:

		// Note: No out-of-bound check for `target_node`
		RotationChannel(uint32_t target_node, Sampler<glm::quat> sampler) :
			target_node(target_node),
			sampler(std::move(sampler))
		{}

		void apply(std::span<Node::TransformOverride> overrides, float time) const noexcept override;
	};

	class ScaleChannel : public Channel
	{
		uint32_t target_node;
		Sampler<glm::vec3> sampler;

	  public:

		// Note: No out-of-bound check for `target_node`
		ScaleChannel(uint32_t target_node, Sampler<glm::vec3> sampler) :
			target_node(target_node),
			sampler(std::move(sampler))
		{}

		void apply(std::span<Node::TransformOverride> overrides, float time) const noexcept override;
	};
}