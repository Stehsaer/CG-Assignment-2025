#pragma once

#include "gpu/command-buffer.hpp"
#include "gpu/compute-pipeline.hpp"
#include "target/gbuffer.hpp"
#include "util/error.hpp"
#include <glm/fwd.hpp>
namespace pipeline
{
	class Hiz_generator
	{
	  public:

		static std::expected<Hiz_generator, util::Error> create(SDL_GPUDevice* device) noexcept;

		std::expected<void, util::Error> generate(
			const gpu::Command_buffer& command_buffer,
			const target::Gbuffer& gbuffer,
			glm::u32vec2 hiz_top_size
		) const noexcept;

	  private:

		struct Param
		{
			glm::i32vec2 src_size;
			glm::i32vec2 dst_size;
		};

		gpu::Compute_pipeline pipeline;

		Hiz_generator(gpu::Compute_pipeline pipeline) :
			pipeline(std::move(pipeline))
		{}

	  public:

		Hiz_generator(const Hiz_generator&) = delete;
		Hiz_generator(Hiz_generator&&) = default;
		Hiz_generator& operator=(const Hiz_generator&) = delete;
		Hiz_generator& operator=(Hiz_generator&&) = default;
	};
}