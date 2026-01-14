#include "gltf/animation.hpp"

#include "gltf/detail/animation/channels.hpp"

namespace gltf
{
	static std::expected<std::unique_ptr<detail::animation::Channel>, util::Error> parse_channel(
		const tinygltf::Model& model,
		const tinygltf::AnimationChannel& channel,
		const tinygltf::AnimationSampler& sampler
	) noexcept
	{
		/* Check Target Index */

		const auto target_node = channel.target_node;
		if (target_node < 0 || std::cmp_greater_equal(target_node, model.nodes.size()))
			return util::Error("Invalid target node index for animation channel");

		/* Get Type */

		const auto& channel_target = channel.target_path;

		if (channel_target == "translation")
		{
			auto sampler_result = detail::animation::Sampler<glm::vec3>::from_tinygltf(model, sampler);
			if (!sampler_result) return sampler_result.error().forward("Parse translation sampler failed");
			return std::make_unique<detail::animation::TranslationChannel>(
				target_node,
				std::move(*sampler_result)
			);
		}
		else if (channel_target == "rotation")
		{
			auto sampler_result = detail::animation::Sampler<glm::quat>::from_tinygltf(model, sampler);
			if (!sampler_result) return sampler_result.error().forward("Parse rotation sampler failed");
			return std::make_unique<detail::animation::RotationChannel>(
				target_node,
				std::move(*sampler_result)
			);
		}
		else if (channel_target == "scale")
		{
			auto sampler_result = detail::animation::Sampler<glm::vec3>::from_tinygltf(model, sampler);
			if (!sampler_result) return sampler_result.error().forward("Parse scale sampler failed");
			return std::make_unique<detail::animation::ScaleChannel>(target_node, std::move(*sampler_result));
		}
		else
			return util::Error(
				std::format("Unknown or unsupported animation channel target path: {}", channel_target)
			);
	}

	std::expected<Animation, util::Error> Animation::from_tinygltf(
		const tinygltf::Model& model,
		const tinygltf::Animation& animation
	) noexcept
	{
		std::vector<std::unique_ptr<detail::animation::Channel>> channels;
		channels.reserve(animation.channels.size());

		for (const auto& channel : animation.channels)
		{
			const auto sampler_index = channel.sampler;
			if (sampler_index < 0 || std::cmp_greater_equal(sampler_index, animation.samplers.size()))
				return util::Error("Invalid sampler index for animation channel");

			auto channel_result = parse_channel(model, channel, animation.samplers[sampler_index]);
			if (!channel_result) return channel_result.error().forward("Parse animation channel failed");

			channels.push_back(std::move(*channel_result));
		}

		return Animation(
			animation.name.empty() ? std::nullopt : std::optional<std::string>(animation.name),
			std::move(channels)
		);
	}

	void Animation::apply(std::span<Node::TransformOverride> overrides, float time) const noexcept
	{
		for (const auto& channel : channels) channel->apply(overrides, time);
	}
}