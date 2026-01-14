#include "gltf/skin.hpp"
#include "gltf/accessor.hpp"
#include "util/as-byte.hpp"

#include <SDL3/SDL_gpu.h>
#include <algorithm>

namespace gltf
{
	///
	/// @brief Parse a tinygltf::Skin into inverse bind matrices and joint indices
	/// @note Validation is already performed. No more assertions needed
	/// @param model Tinygltf model
	/// @param skin Tinygltf skin
	/// @return (`Inverse bind matrices`, `Joint indices`) on success, or error on failure
	///
	static std::expected<std::pair<std::vector<glm::mat4>, std::vector<uint32_t>>, util::Error> parse_skin(
		const tinygltf::Model& model,
		const tinygltf::Skin& skin
	) noexcept
	{
		const auto inv_bind_matrices_idx = skin.inverseBindMatrices;
		if (inv_bind_matrices_idx < 0
			|| std::cmp_greater_equal(inv_bind_matrices_idx, model.accessors.size()))
			return util::Error("Skin inverse bind matrices accessor index out of bounds");

		const auto& inv_bind_matrices_accessor = model.accessors[inv_bind_matrices_idx];
		auto inv_bind_matrices_result = extract_from_accessor<glm::mat4>(model, inv_bind_matrices_accessor);
		if (!inv_bind_matrices_result)
			return inv_bind_matrices_result.error().forward(
				"Extract inverse bind matrices from accessor failed"
			);

		// Validate joint node indices
		if (std::ranges::any_of(skin.joints, [node_count = model.nodes.size()](int idx) {
				return idx < 0 || std::cmp_greater_equal(idx, node_count);
			}))
			return util::Error("Skin joint node index out of bounds");

		// Validate size
		if (inv_bind_matrices_result->size() < skin.joints.size())
			return util::Error("Skin inverse bind matrices count doesn't match joint count");

		// Truncate inverse bind matrices to joint count
		inv_bind_matrices_result->resize(skin.joints.size());

		return std::make_pair(
			std::move(*inv_bind_matrices_result),
			skin.joints | std::views::transform([](int joint_index) {
				return static_cast<uint32_t>(joint_index);
			}) | std::ranges::to<std::vector>()
		);
	}

	std::expected<SkinList, util::Error> SkinList::from_tinygltf(const tinygltf::Model& model) noexcept
	{
		SkinList skin_collection;

		for (const auto [idx, elem] : model.skins | std::views::enumerate)
		{
			auto skin_result = parse_skin(model, elem);
			if (!skin_result) return skin_result.error().forward(std::format("Load skin {} failed", idx));

			skin_collection.skin_offsets
				.emplace_back(skin_collection.joints.size(), skin_result->second.size());

			skin_collection.inverse_bind_matrices.append_range(skin_result->first);
			skin_collection.joints.append_range(skin_result->second);
		}

		return skin_collection;
	}

	std::vector<glm::mat4> SkinList::compute_joint_matrices(
		const std::vector<glm::mat4>& node_world_matrices
	) const noexcept
	{
		std::vector<glm::mat4> joint_matrices;
		joint_matrices.reserve(joints.size());

		for (const auto [inverse_bind_matrix, joint_index] : std::views::zip(inverse_bind_matrices, joints))
		{
			const glm::mat4& node_world_matrix = node_world_matrices[joint_index];
			joint_matrices.emplace_back(node_world_matrix * inverse_bind_matrix);
		}

		return joint_matrices;
	}

	std::expected<void, util::Error> DeferredSkinningResource::prepare_gpu_buffers(
		graphics::BufferPool& buffer_pool,
		graphics::TransferBufferPool& transfer_pool
	) noexcept
	{
		if (upload_buffer || joint_matrices_buffer)
			return util::Error("GPU buffers for skin computation already prepared");

		const auto upload_buffer_result = transfer_pool.acquire_buffer(
			gpu::TransferBuffer::Usage::Upload,
			sizeof(glm::mat4) * joint_matrices_data.size()
		);
		if (!upload_buffer_result)
			return upload_buffer_result.error().forward("Acquire transfer buffer for joint matrices failed");

		const auto buffer_result = buffer_pool.acquire_buffer(
			{.graphic_storage_read = true},
			sizeof(glm::mat4) * joint_matrices_data.size()
		);
		if (!buffer_result) return buffer_result.error().forward("Acquire buffer for joint matrices failed");

		upload_buffer = *upload_buffer_result;
		joint_matrices_buffer = *buffer_result;

		if (const auto result = upload_buffer->upload_to_buffer(util::as_bytes(joint_matrices_data), true);
			!result)
			return result.error().forward("Upload node matrices to transfer buffer failed");

		return {};
	}

	void DeferredSkinningResource::upload_gpu_buffers(const gpu::CopyPass& copy_pass) noexcept
	{
		assert(upload_buffer != nullptr && joint_matrices_buffer != nullptr);

		copy_pass.upload_to_buffer(
			*upload_buffer,
			0,
			*joint_matrices_buffer,
			0,
			sizeof(glm::mat4) * joint_matrices_data.size(),
			true
		);
	}
}