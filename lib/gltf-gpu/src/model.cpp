#include "gltf/model.hpp"

#include <algorithm>
#include <queue>
#include <set>
#include <thread_pool/thread_pool.h>

namespace gltf
{
	static std::expected<std::vector<uint32_t>, util::Error> parse_root_nodes(
		const tinygltf::Model& model
	) noexcept
	{
		uint32_t index;

		if (model.scenes.size() == 1)
			index = 0;
		else
		{
			if (model.defaultScene < 0) return util::Error("No default scene specified with multiple scenes");
			if (std::cmp_greater_equal(model.defaultScene, model.scenes.size()))
				return util::Error("Default scene index out of bounds");

			index = static_cast<uint32_t>(model.defaultScene);
		}

		const auto& nodes = model.scenes[index].nodes;

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

	static std::expected<std::vector<Mesh_gpu>, util::Error> load_meshes(
		SDL_GPUDevice* device,
		const tinygltf::Model& tinygltf_model,
		const std::optional<std::reference_wrapper<std::atomic<Model::Load_progress>>>& progress
	) noexcept
	{
		std::mutex progress_mutex;
		uint32_t progress_count = 0;
		dp::thread_pool thread_pool(std::thread::hardware_concurrency());

		const auto task =
			[device, &progress, &progress_count, &progress_mutex, &tinygltf_model](
				const tinygltf::Mesh& tinygltf_mesh
			) -> std::expected<Mesh_gpu, util::Error> {
			auto mesh_cpu = Mesh::from_tinygltf(tinygltf_model, tinygltf_mesh);
			if (!mesh_cpu) return mesh_cpu.error().propagate("Create mesh from tinygltf failed");

			auto mesh_gpu = Mesh_gpu::from_mesh(device, *mesh_cpu);
			if (!mesh_gpu) return mesh_gpu.error().propagate("Create mesh GPU resources failed");

			{
				std::scoped_lock lock(progress_mutex);
				progress_count++;

				if (progress.has_value())
					progress->get() = {
						.stage = Model::Load_stage::Mesh,
						.progress = float(progress_count) / tinygltf_model.meshes.size()
					};
			}

			return mesh_gpu;
		};

		std::vector<std::future<std::expected<Mesh_gpu, util::Error>>> mesh_futures =
			tinygltf_model.meshes
			| std::views::transform([&](const auto& tinygltf_mesh) {
				  return thread_pool.enqueue(std::bind(task, tinygltf_mesh));
			  })
			| std::ranges::to<std::vector>();

		thread_pool.wait_for_tasks();

		std::vector<Mesh_gpu> meshes;
		for (auto [idx, future] : mesh_futures | std::views::enumerate)
		{
			auto result = future.get();
			if (!result) return result.error().propagate(std::format("Load mesh failed at index {}", idx));
			meshes.emplace_back(std::move(*result));
		}

		return std::move(meshes);
	}

	std::expected<Model, util::Error> Model::load_model(
		SDL_GPUDevice* device,
		const tinygltf::Model& tinygltf_model,
		const Sampler_config& sampler_config,
		const Material_list::Image_config& image_config,
		const std::optional<std::reference_wrapper<std::atomic<Load_progress>>>& progress
	) noexcept
	{
		/* Load Node */

		if (progress) progress->get() = {.stage = Load_stage::Node, .progress = std::nullopt};

		auto root_nodes_result = parse_root_nodes(tinygltf_model);
		if (!root_nodes_result) return root_nodes_result.error().propagate("Parse root nodes failed");

		std::vector<Node> nodes;
		nodes.reserve(tinygltf_model.nodes.size());

		for (const auto& tinygltf_node : tinygltf_model.nodes)
		{
			auto node_result = Node::from_tinygltf(tinygltf_model, tinygltf_node);
			if (!node_result) return node_result.error().propagate("Create node from tinygltf failed");

			nodes.emplace_back(std::move(*node_result));
		}

		/* Load Meshes */

		if (progress) progress->get() = {.stage = Load_stage::Mesh, .progress = 0};

		auto mesh_result = load_meshes(device, tinygltf_model, progress);
		if (!mesh_result) return mesh_result.error().propagate("Load meshes failed");

		/* Load Materials */

		if (progress) progress->get() = {.stage = Load_stage::Material, .progress = 0};
		auto material_list_result = Material_list::create_from_model(
			device,
			tinygltf_model,
			sampler_config,
			image_config,
			[&progress](std::optional<uint32_t> current, uint32_t total) {
				if (!progress) return;
				progress->get() = {
					.stage = Load_stage::Material,
					.progress = current.value_or(0) / float(total == 0 ? 1 : total)
				};
			}
		);
		if (!material_list_result) return material_list_result.error().propagate("Load material failed");

		/* Post Process */

		if (progress) progress->get() = {.stage = Load_stage::Postprocess, .progress = std::nullopt};

		Model model(
			std::move(*material_list_result),
			std::move(*mesh_result),
			std::move(nodes),
			std::move(*root_nodes_result)
		);

		model.compute_node_parents();

		auto topo_order_result = model.compute_topo_order();
		if (!topo_order_result)
			return topo_order_result.error().propagate("Compute node topological order failed");

		model.compute_renderable_nodes();

		return std::move(model);
	}

	Model::Model(
		Material_list material_list,
		std::vector<Mesh_gpu> meshes,
		std::vector<Node> nodes,
		std::vector<uint32_t> root_nodes
	) noexcept :
		material_list(std::move(material_list)),
		meshes(std::move(meshes)),
		nodes(std::move(nodes)),
		root_nodes(std::move(root_nodes)),
		primitive_count(
			std::ranges::fold_left(
				meshes | std::views::transform([](const Mesh_gpu& mesh) { return mesh.primitives.size(); }),
				0zu,
				std::plus()
			)
		)
	{}

	std::vector<Drawdata> Model::generate_drawdata(const glm::mat4& model_transform) const noexcept
	{
		std::vector<glm::mat4> node_world_matrices(nodes.size(), glm::mat4(1.0f));

		for (const auto node_index : node_topo_order)
		{
			const auto& node = nodes[node_index];

			const glm::mat4 parent_matrix =
				node_parents[node_index]
					.transform([&](uint32_t parent_index) { return node_world_matrices[parent_index]; })
					.value_or(model_transform);

			node_world_matrices[node_index] = parent_matrix * node.get_local_transform();
		}

		std::vector<Drawdata> drawdata_list;
		drawdata_list.reserve(primitive_count);

		for (const auto node_index : node_topo_order)
		{
			const auto& node = nodes[node_index];
			if (!node.mesh.has_value()) continue;

			const auto& mesh = meshes[node.mesh.value()];
			const glm::mat4& world_matrix = node_world_matrices[node_index];

			if (!renderable_nodes[node_index]) continue;

			for (const auto& primitive : mesh.primitives)
			{
				const auto material_bind = material_list.gen_binding_info(primitive.material);
				if (!material_bind.has_value()) [[unlikely]]
					continue;  // Skip primitives with invalid material

				drawdata_list.push_back(
					Drawdata{
						.world_matrix = world_matrix,
						.primitive = primitive.gen_drawdata(false),
						.material = *material_bind
					}
				);
			}
		}

		return drawdata_list;
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

		return std::move(model);
	}
}