#include "gltf/model.hpp"
#include "gltf/skin.hpp"
#include "graphics/culling.hpp"

#include <algorithm>
#include <cstdint>
#include <queue>
#include <ranges>
#include <set>
#include <thread_pool/thread_pool.h>

namespace gltf
{
	// Parse root nodes from tinygltf model
	static std::expected<std::vector<uint32_t>, util::Error> parse_root_nodes(
		const tinygltf::Model& model
	) noexcept
	{
		uint32_t index;

		if (model.scenes.size() == 1)  // Only one scene
			index = 0;
		else  // Multiple scenes, select `defaultScene` for root nodes
		{
			if (model.defaultScene < 0) return util::Error("No default scene specified with multiple scenes");
			if (std::cmp_greater_equal(model.defaultScene, model.scenes.size()))
				return util::Error("Default scene index out of bounds");

			index = static_cast<uint32_t>(model.defaultScene);
		}

		const auto& nodes = model.scenes[index].nodes;

		// Out of bound check
		if (std::ranges::any_of(nodes, [&](int node_index) {
				return std::cmp_greater_equal(node_index, model.nodes.size());
			}))
			return util::Error("Scene node index out of bounds");

		return std::vector<uint32_t>(std::from_range, nodes);
	}

	void Model::compute_node_parents() noexcept
	{
		node_parents.resize(nodes.size(), std::nullopt);

		for (const auto& [idx, node] : nodes | std::views::enumerate)
			for (const auto child_index : node.children) node_parents[child_index] = idx;
	}

	void Model::compute_renderable_nodes() noexcept
	{
		renderable_nodes.resize(nodes.size(), false);

		std::queue<uint32_t> process_queue;
		process_queue.push_range(root_nodes);

		while (!process_queue.empty())
		{
			const auto node_index = process_queue.front();
			process_queue.pop();

			renderable_nodes[node_index] = true;

			process_queue.push_range(nodes[node_index].children);
		}
	}

	std::expected<void, util::Error> Model::compute_topo_order() noexcept
	{
		node_topo_order.reserve(nodes.size());

		std::queue<uint32_t> process_queue;
		process_queue.push_range(
			node_parents
			| std::views::enumerate
			| std::views::filter([](const auto& pair) {
				  const auto& [idx, parent] = pair;
				  return parent == std::nullopt;
			  })
			| std::views::keys
		);

		std::set<uint32_t> visited_nodes;

		while (!process_queue.empty())
		{
			const auto node_index = process_queue.front();
			process_queue.pop();

			node_topo_order.push_back(node_index);

			if (visited_nodes.contains(node_index)) return util::Error("Cycle detected in node graph");
			visited_nodes.insert(node_index);

			process_queue.push_range(nodes[node_index].children);
		}

		return {};
	}

	namespace detail
	{
		static std::expected<std::vector<MeshGPU>, util::Error> load_meshes(
			SDL_GPUDevice* device,
			const tinygltf::Model& tinygltf_model,
			const std::optional<std::reference_wrapper<std::atomic<Model::LoadProgress>>>& progress
		) noexcept
		{
			std::mutex progress_mutex;
			uint32_t progress_count = 0;
			dp::thread_pool thread_pool(std::thread::hardware_concurrency());

			const auto task =
				[device, &progress, &progress_count, &progress_mutex, &tinygltf_model](
					const tinygltf::Mesh& tinygltf_mesh
				) -> std::expected<MeshGPU, util::Error> {
				auto mesh_cpu = Mesh::from_tinygltf(tinygltf_model, tinygltf_mesh);
				if (!mesh_cpu) return mesh_cpu.error().forward("Create mesh from tinygltf failed");

				auto mesh_gpu = MeshGPU::from_mesh(device, *mesh_cpu);
				if (!mesh_gpu) return mesh_gpu.error().forward("Create mesh GPU resources failed");

				{
					std::scoped_lock lock(progress_mutex);
					progress_count++;

					if (progress.has_value())
						progress->get() = {
							.stage = Model::LoadStage::Mesh,
							.progress = float(progress_count) / tinygltf_model.meshes.size()
						};
				}

				return mesh_gpu;
			};

			std::vector<std::future<std::expected<MeshGPU, util::Error>>> mesh_futures =
				tinygltf_model.meshes
				| std::views::transform([&](const auto& tinygltf_mesh) {
					  return thread_pool.enqueue(std::bind(task, tinygltf_mesh));
				  })
				| std::ranges::to<std::vector>();

			thread_pool.wait_for_tasks();

			std::vector<MeshGPU> meshes;
			for (auto [idx, future] : mesh_futures | std::views::enumerate)
			{
				auto result = future.get();
				if (!result) return result.error().forward(std::format("Load mesh failed at index {}", idx));
				meshes.emplace_back(std::move(*result));
			}

			return meshes;
		}

		static std::expected<std::vector<Animation>, util::Error> load_animations(
			const tinygltf::Model& tinygltf_model
		) noexcept
		{
			std::vector<Animation> animations;
			animations.reserve(tinygltf_model.animations.size());

			for (const auto& tinygltf_animation : tinygltf_model.animations)
			{
				auto animation_result = Animation::from_tinygltf(tinygltf_model, tinygltf_animation);
				if (!animation_result)
					return animation_result.error().forward("Create animation from tinygltf failed");

				animations.emplace_back(std::move(*animation_result));
			}

			return animations;
		}
	}

	std::expected<Model, util::Error> Model::from_tinygltf(
		SDL_GPUDevice* device,
		const tinygltf::Model& tinygltf_model,
		const SamplerConfig& sampler_config,
		const MaterialList::ImageConfig& image_config,
		const std::optional<std::reference_wrapper<std::atomic<LoadProgress>>>& progress
	) noexcept
	{
		/* Load Node & Lights */

		if (progress) progress->get() = {.stage = LoadStage::Node, .progress = -1};

		auto root_nodes_result = parse_root_nodes(tinygltf_model);
		if (!root_nodes_result) return root_nodes_result.error().forward("Parse root nodes failed");

		std::vector<Node> nodes;
		nodes.reserve(tinygltf_model.nodes.size());

		for (const auto& tinygltf_node : tinygltf_model.nodes)
		{
			auto node_result = Node::from_tinygltf(tinygltf_model, tinygltf_node);
			if (!node_result) return node_result.error().forward("Create node from tinygltf failed");

			nodes.emplace_back(std::move(*node_result));
		}

		std::vector<Light> lights;
		for (const auto& [idx, tinygltf_light] : tinygltf_model.lights | std::views::enumerate)
		{
			auto light_result = parse_light(tinygltf_light);
			if (!light_result)
				return light_result.error().forward(std::format("Parse light failed at index {}", idx));
			lights.emplace_back(*light_result);
		}

		/* Load Meshes */

		if (progress) progress->get() = {.stage = LoadStage::Mesh, .progress = 0};

		auto mesh_result = detail::load_meshes(device, tinygltf_model, progress);
		if (!mesh_result) return mesh_result.error().forward("Load meshes failed");

		/* Load Materials */

		if (progress) progress->get() = {.stage = LoadStage::Material, .progress = 0};
		auto material_list_result = MaterialList::from_tinygltf(
			device,
			tinygltf_model,
			sampler_config,
			image_config,
			[&progress](std::optional<uint32_t> current, uint32_t total) {
				if (!progress) return;
				progress->get() = {
					.stage = LoadStage::Material,
					.progress = current.value_or(0) / float(total == 0 ? 1 : total)
				};
			}
		);
		if (!material_list_result) return material_list_result.error().forward("Load material failed");

		/* Load Animations */

		if (progress) progress->get() = {.stage = LoadStage::Animation, .progress = -1};

		auto animation_result = detail::load_animations(tinygltf_model);
		if (!animation_result) return animation_result.error().forward("Load animations failed");

		/* Load Skins */

		if (progress) progress->get() = {.stage = LoadStage::Skin, .progress = -1};

		auto skin_collection_result = SkinList::from_tinygltf(tinygltf_model);
		if (!skin_collection_result) return skin_collection_result.error().forward("Load skins failed");

		/* Post Process */

		if (progress) progress->get() = {.stage = LoadStage::Postprocess, .progress = -1};

		Model model(
			std::move(*material_list_result),
			std::move(*mesh_result),
			std::move(nodes),
			std::move(*animation_result),
			std::move(*root_nodes_result),
			std::move(*skin_collection_result),
			std::move(lights)
		);

		model.compute_node_parents();

		auto topo_order_result = model.compute_topo_order();
		if (!topo_order_result)
			return topo_order_result.error().forward("Compute node topological order failed");

		model.compute_renderable_nodes();

		auto material_bind_cache_result = model.material_list.gen_material_cache();
		if (!material_bind_cache_result) return util::Error("Generate material bind cache failed");
		model.material_bind_cache = std::move(*material_bind_cache_result);

		return model;
	}

	Model::Model(
		MaterialList material_list,
		std::vector<MeshGPU> meshes,
		std::vector<Node> nodes,
		std::vector<Animation> animations,
		std::vector<uint32_t> root_nodes,
		SkinList skin_collection,
		std::vector<Light> lights
	) noexcept :
		material_list(std::move(material_list)),
		meshes(std::move(meshes)),
		nodes(std::move(nodes)),
		animations(std::move(animations)),
		root_nodes(std::move(root_nodes)),
		skin_list(std::move(skin_collection)),
		lights(std::move(lights)),
		primitive_count(
			std::ranges::fold_left(
				meshes | std::views::transform([](const MeshGPU& mesh) { return mesh.primitives.size(); }),
				0zu,
				std::plus()
			)
		)
	{
		for (auto [idx, animation] : this->animations | std::views::enumerate)
			if (animation.name.has_value()) animation_name_map[*animation.name] = idx;
	}

	std::vector<Node::TransformOverride> Model::compute_node_overrides(
		std::span<const AnimationKey> animation
	) const noexcept
	{
		std::vector<Node::TransformOverride> node_overrides(nodes.size());

		for (const auto& key : animation)
		{
			if (std::holds_alternative<uint32_t>(key.animation))
			{
				const auto animation_index = std::get<uint32_t>(key.animation);
				if (animation_index >= animations.size()) continue;
				animations[animation_index].apply(node_overrides, key.time);
			}
			else
			{
				const auto& animation_name = std::get<std::string>(key.animation);
				const auto it = animation_name_map.find(animation_name);
				if (it == animation_name_map.end()) continue;

				const auto animation_index = it->second;
				if (animation_index >= animations.size()) continue;
				animations[animation_index].apply(node_overrides, key.time);
			}
		}

		return node_overrides;
	}

	std::vector<glm::mat4> Model::compute_node_world_matrices(
		const glm::mat4& model_transform,
		std::span<const Node::TransformOverride> node_overrides
	) const noexcept
	{
		std::vector<glm::mat4> node_world_matrices(nodes.size(), glm::mat4(1.0f));

		for (const auto node_index : node_topo_order)
		{
			const auto& node = nodes[node_index];

			const auto parent_matrix =
				node_parents[node_index]
					.transform([&node_world_matrices](uint32_t parent_index) {
						return std::cref(node_world_matrices[parent_index]);
					})
					.value_or(std::ref(model_transform));

			node_world_matrices[node_index] =
				parent_matrix.get() * node.get_local_transform(node_overrides[node_index]);
		}

		return node_world_matrices;
	}

	std::vector<PrimitiveDrawcall> Model::compute_drawcalls(
		const std::vector<glm::mat4>& node_world_matrices,
		std::span<const std::pair<uint32_t, float>> emission_overrides,
		std::span<const uint32_t> hidden_nodes
	) const noexcept
	{
		std::vector<PrimitiveDrawcall> drawdata_list;
		drawdata_list.reserve(primitive_count);

		auto renderable_nodes = this->renderable_nodes;
		for (const auto hidden_node_index : hidden_nodes) renderable_nodes[hidden_node_index] = false;

		std::vector<float> emission_override_values(nodes.size(), 1.0f);
		for (const auto& [material_index, emission_value] : emission_overrides)
			emission_override_values[material_index] = emission_value;

		for (const auto node_index : node_topo_order)
		{
			const auto& node = nodes[node_index];
			const glm::mat4& world_matrix = node_world_matrices[node_index];

			if (!renderable_nodes[node_index] || !node.mesh.has_value()) continue;

			const auto& mesh = meshes[node.mesh.value()];

			if (node.skin.has_value())  // Rigged
			{
				const auto [_, joints, skin_offset] = skin_list[node.skin.value()];

				const auto node_positions =
					joints
					| std::views::transform([&node_world_matrices](uint32_t joint_index) {
						  const auto& matrix = node_world_matrices[joint_index];
						  const auto& col = matrix[3];
						  return glm::vec3(col.x, col.y, col.z) / col.w;
					  })
					| std::ranges::to<std::vector>();

				const auto world_min = std::ranges::fold_left(
					node_positions,
					glm::vec3(std::numeric_limits<float>::max()),
					[](const glm::vec3& a, const glm::vec3& b) { return glm::min(a, b); }
				);

				const auto world_max = std::ranges::fold_left(
					node_positions,
					glm::vec3(std::numeric_limits<float>::lowest()),
					[](const glm::vec3& a, const glm::vec3& b) { return glm::max(a, b); }
				);

				for (const auto& primitive : mesh.primitives)
				{
					const auto [gen_data, local_min, local_max] = primitive.gen_drawdata();
					const float sphere_diameter = glm::distance(local_min, local_max);

					drawdata_list.emplace_back(
						PrimitiveDrawcall{
							.world_position_min = world_min - glm::vec3(sphere_diameter),
							.world_position_max = world_max + glm::vec3(sphere_diameter),
							.material_index = primitive.material,
							.transform_or_joint_matrix_offset = skin_offset,
							.primitive = gen_data,
						}
					);
				}
			}
			else  // Not Rigged
			{

				for (const auto& primitive : mesh.primitives)
				{
					const auto [gen_data, local_min, local_max] = primitive.gen_drawdata();
					const auto [world_min, world_max] =
						graphics::local_bound_to_world(local_min, local_max, world_matrix);

					drawdata_list.emplace_back(
						PrimitiveDrawcall{
							.world_position_min = world_min,
							.world_position_max = world_max,
							.material_index = primitive.material,
							.transform_or_joint_matrix_offset = world_matrix,
							.primitive = gen_data,
							.emissive_multiplier = emission_override_values[node_index],
						}
					);
				}
			}
		}

		return drawdata_list;
	}

	Drawdata Model::generate_drawdata(
		const glm::mat4& model_transform,
		std::span<const AnimationKey> animation,
		std::span<const std::pair<uint32_t, float>> emission_overrides,
		std::span<const uint32_t> hidden_nodes
	) const noexcept
	{
		const auto node_overrides = compute_node_overrides(animation);
		auto node_world_matrices = compute_node_world_matrices(model_transform, node_overrides);
		auto primitive_list = compute_drawcalls(node_world_matrices, emission_overrides, hidden_nodes);
		auto joint_matrices = skin_list.compute_joint_matrices(node_world_matrices);

		return {
			.primitive_drawcalls = std::move(primitive_list),
			.node_matrices = std::move(node_world_matrices),
			.deferred_skin_resource = joint_matrices.empty()
				? nullptr
				: std::make_shared<DeferredSkinningResource>(std::move(joint_matrices)),
			.material_cache = material_bind_cache->ref()
		};
	}

	std::optional<uint32_t> Model::find_node_by_name(const std::string& name) const noexcept
	{
		const auto found =
			nodes
			| std::views::enumerate
			| std::views::filter([&](const auto& pair) {
				  const auto& [idx, node] = pair;
				  return node.name.has_value() && *node.name == name;
			  })
			| std::ranges::to<std::vector>();

		if (found.size() != 1) return std::nullopt;

		return std::get<0>(found[0]);
	}

	std::optional<std::pair<uint32_t, Light>> Model::find_light_by_name(
		const std::string& name
	) const noexcept
	{
		return find_node_by_name(name).and_then(
			[this](uint32_t node_index) -> std::optional<std::pair<uint32_t, Light>> {
				const auto& node = nodes[node_index];
				if (!node.light.has_value()) return std::nullopt;
				return std::make_pair(node_index, lights[*node.light]);
			}
		);
	}

	std::expected<tinygltf::Model, util::Error> load_tinygltf_model(
		const std::vector<std::byte>& model_data
	) noexcept
	{
		tinygltf::TinyGLTF loader;
		tinygltf::Model model;

		std::string err;
		std::string warn;

		const bool ret = loader.LoadBinaryFromMemory(
			&model,
			&err,
			&warn,
			reinterpret_cast<const unsigned char*>(model_data.data()),
			model_data.size()
		);

		if (!ret) return util::Error(std::format("Load GLTF model failed: {}", err));

		return model;
	}

	std::expected<tinygltf::Model, util::Error> load_tinygltf_model_from_file(
		const std::string& filepath
	) noexcept
	{
		tinygltf::TinyGLTF loader;
		tinygltf::Model model;

		std::string err;
		std::string warn;

		const bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);
		if (!ret)
		{
			// Try binary load
			const bool ret_bin = loader.LoadBinaryFromFile(&model, &err, &warn, filepath);
			if (!ret_bin) return util::Error(std::format("Load GLTF model failed: {}", err));
		}

		return model;
	}
}