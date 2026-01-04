#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <future>
#include <glm/glm.hpp>
#include <imgui.h>
#include <print>

#include "backend/imgui.hpp"
#include "backend/loop.hpp"
#include "backend/sdl.hpp"
#include "logic.hpp"
#include "render.hpp"
#include "util/unwrap.hpp"

static void main_logic(const backend::SDL_context& sdl_context)
{
	auto render_resource =
		backend::display_until_task_done(
			sdl_context,
			std::async(std::launch::async, render::Renderer::create, std::ref(sdl_context)),
			[] {
				ImGui::Text("创建渲染管线...");
				ImGui::ProgressBar(-ImGui::GetTime(), ImVec2(300.0f, 0.0f));
			}
		)
		| util::unwrap("Create render resource failed");

	auto logic = Logic::create(sdl_context) | util::unwrap("Create logic failed");

	/* Main loop */

	bool quit = false;
	bool fullscreen = false;

	while (!quit)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			backend::imgui_handle_event(&event);
			if (event.type == SDL_EVENT_QUIT) quit = true;

			if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F11)
			{
				fullscreen = !fullscreen;
				SDL_SetWindowFullscreen(sdl_context.window, fullscreen);
			}
		}

		/*===== Logic =====*/

		backend::imgui_new_frame();
		auto [params, main_drawdata, primary_point_lights] = logic.logic(sdl_context);

		std::vector<gltf::Drawdata> drawdata_list;
		drawdata_list.emplace_back(std::move(main_drawdata));

		/*===== Render =====*/

		const render::Drawdata drawdata = {.models = drawdata_list, .lights = primary_point_lights};

		render_resource.render(sdl_context, drawdata, params) | util::unwrap("Render frame failed");
	}
}

int main()
try
{
	/* Init */

#ifdef NDEBUG
	constexpr bool enable_debug_layer = false;
#else
	constexpr bool enable_debug_layer = true;
#endif

	backend::initialize_sdl() | util::unwrap("Initialize SDL failed");

	const auto sdl_context =
		backend::SDL_context::create(
			1280,
			720,
			"光线追踪好房子展示程序",
			SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED,
			backend::Vulkan_config{
				.debug_enabled = enable_debug_layer,
			}
		)
		| util::unwrap("Initialize SDL Backend failed");

	const auto window = sdl_context->window;
	SDL_SetWindowMinimumSize(window, 800, 600);

	backend::initialize_imgui(*sdl_context) | util::unwrap();

	main_logic(*sdl_context);

	backend::destroy_imgui();

	return EXIT_SUCCESS;
}
catch (const util::Error& e)
{
	std::println(std::cerr, "\033[91m[Error]\033[0m {}", e->front().message);
	std::println(std::cerr, "===== Stack Trace =====");
	e.dump_trace();
	std::terminate();
}
catch (const std::exception& e)
{
	std::println(std::cerr, "\033[91m[Error]\033[0m {}", e.what());
	std::terminate();
}
catch (...)
{
	std::println(std::cerr, "\033[91m[Error]\033[0m Unknown error");
	std::terminate();
}
