///
/// @file model.hpp
/// @brief Provides a Model class that represents a glTF model, along with Drawcall structs.
///

#pragma once

#include "animation.hpp"
#include "gltf/light.hpp"
#include "gltf/skin.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "node.hpp"

#include <atomic>
#include <cstdint>
#include <unordered_map>
#include <variant>

namespace gltf
{
	// Drawcall for a non-rigged primitive
	struct PrimitiveDrawcall
	{
		glm::vec3 world_position_min;
		glm::vec3 world_position_max;
		std::optional<uint32_t> material_index;
		std::variant<glm::mat4, uint32_t> transform_or_joint_matrix_offset;
		PrimitiveMeshBinding primitive;

		float emissive_multiplier = 1.0f;

		FORCE_INLINE bool is_rigged() const noexcept
		{
			return std::holds_alternative<uint32_t>(transform_or_joint_matrix_offset);
		}

		FORCE_INLINE const glm::mat4& get_world_transform() const noexcept
		{
			return std::get<glm::mat4>(transform_or_joint_matrix_offset);
		}

		FORCE_INLINE uint32_t get_joint_matrix_offset() const noexcept
		{
			return std::get<uint32_t>(transform_or_joint_matrix_offset);
		}
	};

	struct Drawdata
	{
		// Drawcall list
		std::vector<PrimitiveDrawcall> primitive_drawcalls;

		std::vector<glm::mat4> node_matrices;

		// Joint matrices
		std::shared_ptr<DeferredSkinningResource> deferred_skin_resource;

		// Material bind cache reference
		MaterialCache::Ref material_cache;
	};

	class Model
	{
	  private:

		/*===== Resources =====*/

		MaterialList material_list;         // List of materials
		std::vector<MeshGPU> meshes;        // List of meshes
		std::vector<Node> nodes;            // List of nodes
		std::vector<Animation> animations;  // List of animations
		std::vector<uint32_t> root_nodes;   // List of root node indices
		SkinList skin_list;                 // Collection of skins
		std::vector<Light> lights;          // List of lights

		/*===== Accelerating Structures =====*/

		std::vector<uint32_t> node_topo_order;                // Topological order of node indices
		std::vector<std::optional<uint32_t>> node_parents;    // Parent index for each node
		std::vector<bool> renderable_nodes;                   // If node is renderable (children of root)
		size_t primitive_count;                               // Total primitive count
		std::unique_ptr<MaterialCache> material_bind_cache;   // Material bind cache
		std::unordered_map<std::string, uint32_t> animation_name_map;  // Map of animation name to index

	  public:

		enum class LoadStage
		{
			Node,
			Mesh,
			Material,
			Animation,
			Skin,
			Postprocess
		};

		struct LoadProgress
		{
			LoadStage stage;
			float progress;  // Negative => indeterminate
		};

		///
		/// @brief Load model from tinygltf model
		///
		/// @param tinygltf_model Tinygltf model
		/// @param sampler_config Sampler creation config
		/// @param image_config Image compression config
		/// @param progress Progress reference for loading progress (optional)
		/// @return Loaded Model or Error
		///
		static std::expected<Model, util::Error> from_tinygltf(
			SDL_GPUDevice* device,
			const tinygltf::Model& tinygltf_model,
			const SamplerConfig& sampler_config,
			const MaterialList::ImageConfig& image_config,
			const std::optional<std::reference_wrapper<std::atomic<LoadProgress>>>& progress = std::nullopt
		) noexcept;

		///
		/// @brief Generate drawdata for the model
		/// @warning The life span of the returned drawdata is shorter than the life span of the model
		///
		/// @param model_transform Root model transform matrix
		/// @param animation Animation keys to apply
		/// @param emission_overrides Overrides for emissive factors (node_index, multiplier)
		/// @param hidden_nodes List of node indices to hide
		/// @return Drawdata, where drawcall's matrix denotes `Model->World` transform
		///
		Drawdata generate_drawdata(
			const glm::mat4& model_transform,
			std::span<const AnimationKey> animation,
			std::span<const std::pair<uint32_t, float>> emission_overrides,
			std::span<const uint32_t> hidden_nodes
		) const noexcept;

		///
		/// @brief Get the list of animations
		///
		/// @return List of animations
		///
		std::span<const Animation> get_animations() const noexcept { return animations; }

		///
		/// @brief Find a unique node by name
		///
		/// @param name Name of the node
		/// @return If found and unique, the node index; otherwise, nullopt
		///
		std::optional<uint32_t> find_node_by_name(const std::string& name) const noexcept;

		///
		/// @brief Get (node_index, Light) by name
		///
		/// @return If found, a (node_index, Light) pair
		///
		std::optional<std::pair<uint32_t, Light>> find_light_by_name(const std::string& name) const noexcept;

	  private:

		/*===== Load Stage =====*/

		// Compute parent indices for all nodes.
		void compute_node_parents() noexcept;

		// Compute topological order of nodes, must be called after `compute_node_parents()`.
		std::expected<void, util::Error> compute_topo_order() noexcept;

		// Compute which nodes are renderable (children of root nodes). No loop checks are performed and must
		// be called after `compute_topo_order()`.
		void compute_renderable_nodes() noexcept;

		/*===== Render Stage =====*/

		// Compute node transform overrides from animation keys
		std::vector<Node::TransformOverride> compute_node_overrides(
			std::span<const AnimationKey> animation
		) const noexcept;

		// Compute world matrices for all nodes
		std::vector<glm::mat4> compute_node_world_matrices(
			const glm::mat4& model_transform,
			std::span<const Node::TransformOverride> node_overrides
		) const noexcept;

		// Generate drawcalls from world matrices
		std::vector<PrimitiveDrawcall> compute_drawcalls(
			const std::vector<glm::mat4>& node_world_matrices,
			std::span<const std::pair<uint32_t, float>> emission_overrides,
			std::span<const uint32_t> hidden_nodes
		) const noexcept;

		Model(
			MaterialList material_list,
			std::vector<MeshGPU> meshes,
			std::vector<Node> nodes,
			std::vector<Animation> animations,
			std::vector<uint32_t> root_nodes,
			SkinList skin_collection,
			std::vector<Light> lights
		) noexcept;

	  public:

		Model(const Model&) = delete;
		Model& operator=(const Model&) = delete;
		Model(Model&&) = default;
		Model& operator=(Model&&) = default;
		~Model() = default;
	};

	///
	/// @brief Load tinygltf model from binary glTF data
	///
	/// @param model_data Binary glTF data
	/// @return tinygltf Model on success, or Error on failure
	///
	std::expected<tinygltf::Model, util::Error> load_tinygltf_model(
		const std::vector<std::byte>& model_data
	) noexcept;

	///
	/// @brief Load tinygltf model from file
	///
	/// @param filepath Path to glTF file
	/// @return tinygltf Model on success, or Error on failure
	///
	std::expected<tinygltf::Model, util::Error> load_tinygltf_model_from_file(
		const std::string& filepath
	) noexcept;
}