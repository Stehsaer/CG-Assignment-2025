#pragma once

#include "gpu/graphics-pipeline.hpp"

#include <glm/glm.hpp>

namespace pipeline
{
	class Line
	{
	  public:

		struct Param
		{
			glm::mat4 vp_matrix;
		};

		static std::expected<Line, util::Error> create(
			SDL_GPUDevice* device,
			SDL_GPUTextureFormat swapchain_format
		) noexcept;

		gpu::GraphicsPipeline pipeline;

	  private:

		Line(gpu::GraphicsPipeline pipeline) :
			pipeline(std::move(pipeline))
		{}

	  public:

		Line(const Line&) = delete;
		Line(Line&&) = default;
		Line& operator=(const Line&) = delete;
		Line& operator=(Line&&) = default;
	};
}