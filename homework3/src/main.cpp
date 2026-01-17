#include "render.hpp"
#include "util/error.hpp"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <imgui.h>
#include <print>

int main()
{

	try
	{
		RenderManager render_manager = RenderManager::create();
		while (render_manager.run_frame());

		return EXIT_SUCCESS;
	}
	catch (const util::Error& err)
	{
		err.dump_trace();
		return EXIT_FAILURE;
	}
	catch (const std::exception& e)
	{
		std::println("Error: {}", e.what());
		return EXIT_FAILURE;
	}
}