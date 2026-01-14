#pragma once

#include "gpu/buffer.hpp"
#include "gpu/copy-pass.hpp"
#include "graphics/util/buffer-pool.hpp"
#include "util/error.hpp"
#include "util/inline.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <tiny_gltf.h>

namespace gltf
{
	struct Skin
	{
		std::span<const glm::mat4> inverse_bind_matrices;
		std::span<const uint32_t> joints;
		uint32_t offset;
	};

	// Collection of skins
	struct SkinList
	{
		std::vector<glm::mat4> inverse_bind_matrices;
		std::vector<uint32_t> joints;

		// Skin binding (offset, length) by skin index
		std::vector<std::pair<uint32_t, uint32_t>> skin_offsets;

		static std::expected<SkinList, util::Error> from_tinygltf(const tinygltf::Model& model) noexcept;

		std::vector<glm::mat4> compute_joint_matrices(
			const std::vector<glm::mat4>& node_world_matrices
		) const noexcept;

		FORCE_INLINE Skin operator[](size_t idx) const noexcept
		{
			const auto [offset, length] = skin_offsets[idx];
			return {
				.inverse_bind_matrices = std::span(inverse_bind_matrices).subspan(offset, length),
				.joints = std::span(joints).subspan(offset, length),
				.offset = offset
			};
		}
	};

	///
	/// @brief Deferred Skinning Resource
	/// @details
	/// - Holds resource for deferred skinning computation. The external renderer and the drawdata
	/// unit share this resource
	/// - Responsible for preparing and uploading GPU buffers for skin computation
	///
	struct DeferredSkinningResource
	{
		std::vector<glm::mat4> joint_matrices_data;

		// Initialize at render time, see `prepare_gpu_buffers`
		std::shared_ptr<gpu::TransferBuffer> upload_buffer = nullptr;

		// Initialize at render time, see `prepare_gpu_buffers`
		std::shared_ptr<gpu::Buffer> joint_matrices_buffer = nullptr;

		///
		/// @brief Constructs a skinning resource with joint matrices data
		///
		/// @param joint_matrices_data Computed joint matrices data, see `Skin_list::compute_joint_matrices`
		///
		DeferredSkinningResource(std::vector<glm::mat4> joint_matrices_data) :
			joint_matrices_data(std::move(joint_matrices_data))
		{}

		///
		/// @brief Acquire GPU buffers for skin computation
		///
		/// @param buffer_pool Buffer Pool
		/// @return Void on success, or error on failure
		///
		std::expected<void, util::Error> prepare_gpu_buffers(
			graphics::BufferPool& buffer_pool,
			graphics::TransferBufferPool& transfer_pool
		) noexcept;

		///
		/// @brief Upload GPU buffers
		///
		/// @param copy_pass Copy Pass
		///
		void upload_gpu_buffers(const gpu::CopyPass& copy_pass) noexcept;
	};
}