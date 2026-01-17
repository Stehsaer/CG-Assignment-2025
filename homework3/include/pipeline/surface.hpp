#pragma once

#include "gpu/buffer.hpp"
#include "gpu/command-buffer.hpp"
#include "gpu/graphics-pipeline.hpp"
#include "gpu/render-pass.hpp"
#include <glm/glm.hpp>

namespace pipeline
{
	class Surface
	{
	  public:

		struct Params
		{
			glm::mat4 vp_matrix;
			glm::mat4 control_points;
		};

		static std::expected<Surface, util::Error> create(SDL_GPUDevice* device) noexcept;

		void draw(
			const gpu::CommandBuffer& command_buffer,
			const gpu::RenderPass& render_pass,
			const Params& params,
			bool wireframe
		) const noexcept;

	  private:

		gpu::GraphicsPipeline solid_pipeline;
		gpu::GraphicsPipeline wireframe_pipeline;

		gpu::Buffer vertex_buffer, index_buffer;

		static constexpr size_t SURFACE_RES = 64;

		Surface(
			gpu::GraphicsPipeline solid_pipeline,
			gpu::GraphicsPipeline wireframe_pipeline,
			gpu::Buffer vertex_buffer,
			gpu::Buffer index_buffer
		) :
			solid_pipeline(std::move(solid_pipeline)),
			wireframe_pipeline(std::move(wireframe_pipeline)),
			vertex_buffer(std::move(vertex_buffer)),
			index_buffer(std::move(index_buffer))
		{}

	  public:

		Surface(const Surface&) = delete;
		Surface(Surface&&) = default;
		Surface& operator=(const Surface&) = delete;
		Surface& operator=(Surface&&) = default;
	};
}
