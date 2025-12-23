#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <future>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/matrix.hpp>
#include <imgui.h>
#include <iostream>
#include <print>

#include "backend/imgui.hpp"
#include "backend/loop.hpp"
#include "backend/sdl.hpp"
#include "gltf/model.hpp"
#include "logic.hpp"
#include "render.hpp"
#include "tiny_gltf.h"
#include "util/unwrap.hpp"

static std::expected<gltf::Model, util::Error> create_scene_from_model(
	const backend::SDL_context& context,
	const std::string& path
) noexcept
{
	auto gltf_load_result = backend::display_until_task_done(
		context,
		std::async(std::launch::async, gltf::load_tinygltf_model_from_file, std::ref(path)),
		[] {
			ImGui::Text("加载模型...");
			ImGui::ProgressBar(-ImGui::GetTime(), ImVec2(300.0f, 0.0f));
		}
	);
	if (!gltf_load_result) return gltf_load_result.error().forward("Load tinygltf model failed");

	std::atomic<gltf::Model::Load_progress> load_progress;

	auto future = std::async(std::launch::async, [&context, &gltf_load_result, &load_progress]() {
		return gltf::Model::from_tinygltf(
			context.device,
			*gltf_load_result,
			gltf::Sampler_config{.anisotropy = 4.0f},
			{.color_mode = gltf::Color_compress_mode::RGBA8_BC3,
			 .normal_mode = gltf::Normal_compress_mode::RGn_BC5},
			std::ref(load_progress)
		);
	});

	auto gltf_result = backend::display_until_task_done(context, std::move(future), [&load_progress] {
		const auto current = load_progress.load();

		switch (current.stage)
		{
		case gltf::Model::Load_stage::Node:
			ImGui::Text("解析节点树...");
			break;
		case gltf::Model::Load_stage::Mesh:
			ImGui::Text("分析并优化网格...");
			break;
		case gltf::Model::Load_stage::Material:
			ImGui::Text("压缩材质...");
			break;
		case gltf::Model::Load_stage::Animation:
			ImGui::Text("解析动画...");
			break;
		case gltf::Model::Load_stage::Skin:
			ImGui::Text("解析皮肤...");
			break;
		case gltf::Model::Load_stage::Postprocess:
			ImGui::Text("处理中...");
			break;
		}

		ImGui::ProgressBar(current.progress.value_or(-ImGui::GetTime()), ImVec2(300.0f, 0.0f));
	});
	if (!gltf_result) return gltf_result.error().forward("Load gltf model failed");

	return gltf_result;
}

static void main_logic(const backend::SDL_context& sdl_context, const std::string& model_path)
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

	auto model = create_scene_from_model(sdl_context, model_path) | util::unwrap("Load 3D model failed");

	Logic logic;

	/* 主循环 */

	bool quit = false;
	while (!quit)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			backend::imgui_handle_event(&event);
			if (event.type == SDL_EVENT_QUIT) quit = true;
		}

		/*===== Logic =====*/

		backend::imgui_new_frame();
		const auto [params, drawdata] = logic.logic(sdl_context, model);

		/*===== Render =====*/

		render_resource.render(sdl_context, drawdata, params) | util::unwrap("Render frame failed");
	}
}

int main(int argc, const char** argv)
try
{
	std::span<const char*> args(argv, argc);
	if (args.size() != 2)
	{
		std::println(std::cerr, "No path specified!");
		return EXIT_FAILURE;
	}

	/* 初始化 */

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
			"Demo",
			SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED,
			backend::Vulkan_config{
				.debug_enabled = enable_debug_layer,
			}
		)
		| util::unwrap("Initialize SDL Backend failed");

	const auto window = sdl_context->window;
	SDL_SetWindowMinimumSize(window, 800, 600);

	backend::initialize_imgui(*sdl_context) | util::unwrap();

	main_logic(*sdl_context, args[1]);

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
	std::println(std::cerr, "\033[91m[异常]\033[0m {}", e.what());
	std::terminate();
}
catch (...)
{
	std::println(std::cerr, "\033[91m[异常]\033[0m 未知异常发生");
	std::terminate();
}
