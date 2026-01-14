///
/// @file mesh.hpp
/// @brief Provides structs repesenting vertices, primitives and meshes, both on CPU and GPU sides.
///

#pragma once

#include "gpu/buffer.hpp"
#include "util/inline.hpp"

#include <glm/glm.hpp>
#include <optional>
#include <tiny_gltf.h>
#include <vector>

namespace gltf
{
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 texcoord;

		bool operator==(const Vertex& other) const noexcept;
	};

	struct RiggedVertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 texcoord;
		glm::uvec4 joint_indices;
		glm::vec4 joint_weights;

		bool operator==(const RiggedVertex& other) const noexcept;
	};

	struct ShadowVertex
	{
		glm::vec3 position;
		glm::vec2 texcoord;

		bool operator==(const ShadowVertex& other) const noexcept;

		static ShadowVertex from_vertex(const Vertex& vertex) noexcept;
	};

	struct RiggedShadowVertex
	{
		glm::vec3 position;
		glm::vec2 texcoord;
		glm::uvec4 joint_indices;
		glm::vec4 joint_weights;

		bool operator==(const RiggedShadowVertex& other) const noexcept;

		static RiggedShadowVertex from_rigged_vertex(const RiggedVertex& vertex) noexcept;
	};

	// Primitive Mesh Data
	struct Primitive
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		std::vector<ShadowVertex> shadow_vertices;
		std::vector<uint32_t> shadow_indices;

		std::optional<uint32_t> material;

		glm::vec3 position_min, position_max;

		///
		/// @brief Create a `Primitive` from a `tinygltf::Primitive`, performing validation
		///
		/// @param model Tinygltf model
		/// @param primitive Tinygltf primitive
		/// @return Primitive on success, or error on failure
		///
		static std::expected<Primitive, util::Error> from_tinygltf(
			const tinygltf::Model& model,
			const tinygltf::Primitive& primitive
		) noexcept;
	};

	// Rigged Primitive Mesh Data
	struct RiggedPrimitive
	{
		std::vector<RiggedVertex> vertices;
		std::vector<uint32_t> indices;

		std::vector<RiggedShadowVertex> shadow_vertices;
		std::vector<uint32_t> shadow_indices;

		std::optional<uint32_t> material;

		glm::vec3 position_min, position_max;

		///
		/// @brief Create a `Rigged_primitive` from a `tinygltf::Primitive`, performing validation
		///
		/// @param model Tinygltf model
		/// @param primitive Tinygltf primitive
		/// @return Rigged_primitive on success, or error on failure
		///
		static std::expected<RiggedPrimitive, util::Error> from_tinygltf(
			const tinygltf::Model& model,
			const tinygltf::Primitive& primitive
		) noexcept;
	};

	// Plain raw data for drawing a primitive
	struct PrimitiveMeshBinding
	{
		SDL_GPUBufferBinding vertex_buffer_binding;
		SDL_GPUBufferBinding index_buffer_binding;
		SDL_GPUBufferBinding shadow_vertex_buffer_binding;
		SDL_GPUBufferBinding shadow_index_buffer_binding;
		uint32_t index_count;
		bool rigged;
	};

	// Primitive Mesh Data for GPU
	struct PrimitiveGPU
	{
		uint32_t index_count;

		gpu::Buffer vertex_buffer;
		gpu::Buffer index_buffer;
		gpu::Buffer shadow_vertex_buffer;
		gpu::Buffer shadow_index_buffer;

		std::optional<uint32_t> material;
		glm::vec3 position_min, position_max;
		bool rigged;

		///
		/// @brief Create a `Primitive_gpu` from a `Primitive`, uploading data to the GPU
		///
		/// @param primitive CPU-side primitive
		/// @return GPU-side primitive, or error on failure
		///
		static std::expected<PrimitiveGPU, util::Error> from_primitive(
			SDL_GPUDevice* device,
			const Primitive& primitive
		) noexcept;

		///
		/// @brief Create a `Primitive_gpu` from a `Rigged_primitive`, uploading data to the GPU
		///
		/// @param primitive CPU-side rigged primitive
		/// @return GPU-side primitive, or error on failure
		///
		static std::expected<PrimitiveGPU, util::Error> from_rigged_primitive(
			SDL_GPUDevice* device,
			const RiggedPrimitive& primitive
		) noexcept;

		///
		/// @brief Generate drawdata for this primitive
		/// @return (Primitive_draw, local_position_min, local_position_max)
		///
		FORCE_INLINE std::tuple<PrimitiveMeshBinding, glm::vec3, glm::vec3> gen_drawdata() const noexcept
		{
			return {
				{.vertex_buffer_binding = {.buffer = vertex_buffer, .offset = 0},
				 .index_buffer_binding = {.buffer = index_buffer, .offset = 0},
				 .shadow_vertex_buffer_binding = {.buffer = shadow_vertex_buffer, .offset = 0},
				 .shadow_index_buffer_binding = {.buffer = shadow_index_buffer, .offset = 0},
				 .index_count = index_count,
				 .rigged = rigged},
				position_min,
				position_max
			};
		}
	};

	// Mesh data on CPU side
	struct Mesh
	{
		std::vector<Primitive> primitives;
		std::vector<RiggedPrimitive> rigged_primitives;

		///
		/// @brief Parse a `tinygltf::Mesh` into a `Mesh`
		///
		/// @param model Tinygltf model
		/// @param mesh Tinygltf mesh
		/// @return Mesh on success, or error on failure
		///
		static std::expected<Mesh, util::Error> from_tinygltf(
			const tinygltf::Model& model,
			const tinygltf::Mesh& mesh
		) noexcept;
	};

	// Mesh data on GPU side
	struct MeshGPU
	{
		std::vector<PrimitiveGPU> primitives;

		///
		/// @brief Upload a `Mesh` to GPU, creating `Mesh_gpu`
		///
		/// @param mesh CPU-side mesh
		/// @return GPU-side mesh, or error on failure
		///
		static std::expected<MeshGPU, util::Error> from_mesh(
			SDL_GPUDevice* device,
			const Mesh& mesh
		) noexcept;
	};
}