#include "pipeline/line.hpp"
#include "geometry/vertex.hpp"
#include "gpu/graphics-pipeline.hpp"
#include "shader/line.hpp"
#include "target/msaa-draw.hpp"

#include <SDL3/SDL_gpu.h>

namespace pipeline
{
	std::expected<Line, util::Error> Line::create(SDL_GPUDevice* device) noexcept
	{
		const auto shader_vertex = gpu::GraphicsShader::create(
			device,
			shader::line,
			gpu::GraphicsShader::Stage::Vertex,
			0,
			0,
			0,
			1,
			"vs_main"
		);

		const auto shader_fragment = gpu::GraphicsShader::create(
			device,
			shader::line,
			gpu::GraphicsShader::Stage::Fragment,
			0,
			0,
			0,
			0,
			"fs_main"
		);

		if (!shader_vertex) return shader_vertex.error();
		if (!shader_fragment) return shader_fragment.error();

		const auto swapchain_desc = SDL_GPUColorTargetDescription{
			.format = target::MSAADraw::FORMAT.format,
			.blend_state = {
							.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
							.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
							.color_blend_op = SDL_GPU_BLENDOP_ADD,
							.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
							.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
							.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
							.color_write_mask = {},
							.enable_blend = true,
							.enable_color_write_mask = false,
							.padding1 = 0,
							.padding2 = 0
			},
		};
		const auto color_target_descs = std::to_array({swapchain_desc});

		auto pipeline = gpu::GraphicsPipeline::create(
			device,
			*shader_vertex,
			*shader_fragment,
			SDL_GPU_PRIMITIVETYPE_LINESTRIP,
			target::MSAADraw::MSAA_SAMPLES,
			SDL_GPURasterizerState{
				.fill_mode = SDL_GPU_FILLMODE_LINE,
				.cull_mode = SDL_GPU_CULLMODE_NONE,
				.front_face = {},
				.depth_bias_constant_factor = 0,
				.depth_bias_clamp = 0,
				.depth_bias_slope_factor = 0,
				.enable_depth_bias = false,
				.enable_depth_clip = false,
				.padding1 = 0,
				.padding2 = 0
			},
			LineVertex::attributes,
			LineVertex::buffer_description,
			color_target_descs,
			std::nullopt,
			"Line Pipeline"
		);

		if (!pipeline) return pipeline.error();
		return Line(std::move(*pipeline));
	}
}