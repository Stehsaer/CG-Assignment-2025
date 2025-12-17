#pragma once

#include "gltf/material.hpp"
#include "gltf/model.hpp"
#include "pipeline/gbuffer-gltf.hpp"
#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

namespace renderer
{
	///
	/// @brief Gbuffer renderer for GLTF drawcalls
	///
	class Gbuffer_gltf
	{
	  public:

		///
		/// @brief Drawdata for Gbuffer rendering
		/// @details Contains culled drawcalls and camera info, use `add` to append glTF drawdata
		///
		class Drawdata
		{
			struct Drawcall
			{
				gltf::Primitive_drawcall drawcall;
				size_t resource_set_index;
				float max_z;
			};

			struct Resource
			{
				gltf::Material_cache::Ref material_cache;
				std::shared_ptr<gltf::Deferred_skinning_resource> deferred_skinning_resource;
			};

			std::map<std::pair<gltf::Pipeline_mode, bool>, std::vector<Drawcall>> drawcalls;
			std::vector<Resource> resource_sets;

			glm::mat4 camera_matrix;
			glm::vec3 eye_position;
			glm::vec3 eye_to_nearplane;

			std::array<glm::vec4, 6> frustum_planes;
			float min_z = 1;      // Minimum Z value
			float near_distance;  // Distance from eye to near plane

			friend Gbuffer_gltf;

		  public:

			///
			/// @brief Create drawdata with camera matrix
			///
			/// @param camera_matrix Camera matrix
			///
			Drawdata(const glm::mat4& camera_matrix, const glm::vec3& eye_position) noexcept;

			///
			/// @brief Add glTF drawdata
			///
			/// @param drawdata glTF drawdata
			///
			void append(const gltf::Drawdata& drawdata) noexcept;

			///
			/// @brief Get maximum z depth
			///
			/// @return Maximum z depth
			///
			float get_min_z() const noexcept;

			///
			/// @brief Get absolute maximum distance of all drawcalls
			/// @note Calculated from camera position to the farthest visible point at min-z plane
			/// @return Maximum distance
			///
			float get_max_distance() const noexcept;

			///
			/// @brief Sort drawcalls for optimal rendering
			///
			///
			void sort() noexcept;
		};

		///
		/// @brief Create a gbuffer render renderer
		///
		/// @return Created Gbuffer renderer, or error on failure
		///
		static std::expected<Gbuffer_gltf, util::Error> create(SDL_GPUDevice* device) noexcept;

		///
		/// @brief Render the drawdata into the gbuffer pass
		///
		/// @param command_buffer Target command buffer
		/// @param gbuffer_pass Target gbuffer render pass
		/// @param drawdata Drawdata to render
		///
		void render(
			const gpu::Command_buffer& command_buffer,
			const gpu::Render_pass& gbuffer_pass,
			const Drawdata& drawdata
		) const noexcept;

	  private:

		pipeline::Gbuffer_gltf pipeline;

		Gbuffer_gltf(pipeline::Gbuffer_gltf pipeline) noexcept :
			pipeline(std::move(pipeline))
		{}

	  public:

		Gbuffer_gltf(const Gbuffer_gltf&) = delete;
		Gbuffer_gltf(Gbuffer_gltf&&) = default;
		Gbuffer_gltf& operator=(const Gbuffer_gltf&) = delete;
		Gbuffer_gltf& operator=(Gbuffer_gltf&&) = default;
	};
}