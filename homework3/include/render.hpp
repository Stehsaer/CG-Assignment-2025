#pragma once

#include "backend/sdl.hpp"
#include "logic.hpp"
#include "pipeline/line.hpp"
#include "pipeline/surface.hpp"
#include "target/msaa-draw.hpp"

class RenderManager
{
  public:

	// Creates render manager, may throw util::Error
	static RenderManager create();

	///
	/// @brief Runs a single frame, may throw util::Error
	///
	/// @retval true continue running
	/// @retval false quit application
	///
	bool run_frame();

  private:

	std::unique_ptr<backend::SDLcontext> sdl_context;
	pipeline::Line line_pipeline;
	pipeline::Surface surface_pipeline;
	target::MSAADraw msaa_buffer;
	logic::App app;

	RenderManager(
		std::unique_ptr<backend::SDLcontext> sdl_context,
		pipeline::Line line_pipeline,
		pipeline::Surface surface_pipeline,
		target::MSAADraw msaa_buffer
	) :
		sdl_context(std::move(sdl_context)),
		line_pipeline(std::move(line_pipeline)),
		surface_pipeline(std::move(surface_pipeline)),
		msaa_buffer(std::move(msaa_buffer))
	{}

  public:

	RenderManager(const RenderManager&) = delete;
	RenderManager(RenderManager&&) = default;
	RenderManager& operator=(const RenderManager&) = delete;
	RenderManager& operator=(RenderManager&&) = delete;
};