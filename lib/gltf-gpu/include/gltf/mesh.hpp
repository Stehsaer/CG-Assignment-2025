#pragma once

#include <glm/glm.hpp>
#include <gpu/buffer.hpp>
#include <optional>
#include <tiny_gltf.h>
#include <util/inline.hpp>
#include <vector>

namespace gltf
{
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 texcoord;
	};

	// Primitive Mesh Data
	struct Primitive
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		std::vector<glm::vec3> position_vertices;
		std::vector<uint32_t> position_indices;

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

	// Plain raw data for drawing a primitive
	struct Primitive_draw
	{
		SDL_GPUBufferBinding vertex_buffer_binding;
		SDL_GPUBufferBinding index_buffer_binding;
		uint32_t index_count;
		glm::vec3 position_min;
		glm::vec3 position_max;
	};

	// Primitive Mesh Data for GPU
	struct Primitive_gpu
	{
		uint32_t index_count;

		gpu::Buffer vertex_buffer;
		gpu::Buffer index_buffer;
		gpu::Buffer position_vertex_buffer;
		gpu::Buffer position_index_buffer;

		std::optional<uint32_t> material;
		glm::vec3 position_min, position_max;

		///
		/// @brief Create a `Primitive_gpu` from a `Primitive`, uploading data to the GPU
		///
		/// @param primitive CPU-side primitive
		/// @return GPU-side primitive, or error on failure
		///
		static std::expected<Primitive_gpu, util::Error> from_primitive(
			SDL_GPUDevice* device,
			const Primitive& primitive
		) noexcept;

		// Generate drawdata for this primitive
		FORCE_INLINE Primitive_draw gen_drawdata(bool position_only) const noexcept
		{
			return Primitive_draw{
				.vertex_buffer_binding =
					{.buffer = position_only ? position_vertex_buffer : vertex_buffer, .offset = 0},
				.index_buffer_binding =
					{.buffer = position_only ? position_index_buffer : index_buffer,   .offset = 0},
				.index_count = index_count,
				.position_min = position_min,
				.position_max = position_max
			};
		}
	};

	struct Mesh
	{
		std::vector<Primitive> primitives;

		static std::expected<Mesh, util::Error> from_tinygltf(
			const tinygltf::Model& model,
			const tinygltf::Mesh& mesh
		) noexcept;
	};

	struct Mesh_gpu
	{
		std::vector<Primitive_gpu> primitives;

		static std::expected<Mesh_gpu, util::Error> from_mesh(
			SDL_GPUDevice* device,
			const Mesh& mesh
		) noexcept;
	};
}