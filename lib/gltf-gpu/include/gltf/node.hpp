#pragma once

#include <expected>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>
#include <tiny_gltf.h>
#include <util/error.hpp>
#include <util/inline.hpp>
#include <variant>
#include <vector>

namespace gltf
{
	struct Node
	{
		struct Transform_override
		{
			std::optional<glm::vec3> translation;
			std::optional<glm::quat> rotation;
			std::optional<glm::vec3> scale;

			FORCE_INLINE bool has_override() const noexcept
			{
				return translation.has_value() || rotation.has_value() || scale.has_value();
			}
		};

		struct Transform
		{
			glm::vec3 translation{0.0f, 0.0f, 0.0f};
			glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
			glm::vec3 scale{1.0f, 1.0f, 1.0f};

			///
			/// @brief Get the overriden transform
			///
			/// @param override Override for translation
			/// @return The new Transform with overrides applied
			///
			FORCE_INLINE Transform override_with(const Transform_override& override) const noexcept
			{
				return {
					.translation = override.translation.value_or(translation),
					.rotation = override.rotation.value_or(rotation),
					.scale = override.scale.value_or(scale)
				};
			}

			///
			/// @brief Convert the Transform to a 4x4 matrix. T*R*S order.
			///
			/// @return The 4x4 transformation matrix
			///
			FORCE_INLINE glm::mat4 to_matrix() const noexcept
			{
				const glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), translation);
				const auto rotation_matrix = glm::mat4(rotation);
				const glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), scale);

				return translation_matrix * rotation_matrix * scale_matrix;
			}
		};

		std::vector<uint32_t> children;
		std::optional<uint32_t> mesh = std::nullopt;
		std::optional<uint32_t> skin = std::nullopt;
		std::variant<Transform, glm::mat4> transform = Transform{};

		// TODO: Weights, Name

		///
		/// @brief Parse a glTF node from tinygltf representation
		/// @note Indexes are already validated to be in range, use it directly
		///
		/// @param model Tinygltf model
		/// @param node Tinygltf node
		/// @return Node, or error if parsing failed
		///
		static std::expected<Node, util::Error> from_tinygltf(
			const tinygltf::Model& model,
			const tinygltf::Node& node
		) noexcept;

		///
		/// @brief Get local transformation matrix with optional overrides
		///
		/// @param translation_override Translation override
		/// @param rotation_override Rotation override
		/// @param scale_override Scale override
		/// @return Local transformation matrix
		///
		FORCE_INLINE glm::mat4 get_local_transform(const Transform_override& override = {}) const noexcept
		{
			if (std::holds_alternative<glm::mat4>(transform)) [[unlikely]]
			{
				if (!override.has_override()) [[likely]]
					return std::get<glm::mat4>(transform);
				else
					return Transform().override_with(override).to_matrix();
			}
			else
			{
				if (!override.has_override()) [[likely]]
					return std::get<Transform>(transform).to_matrix();
				else
					return std::get<Transform>(transform).override_with(override).to_matrix();
			}
		}
	};
};