#include <SDL3/SDL_events.h>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <iostream>
#include <print>

#include "asset/graphic-asset.hpp"
#include "asset/shader/simple.frag.hpp"
#include "asset/shader/simple.vert.hpp"

#include "config/general.hpp"
#include "config/texture.hpp"

#include <backend/imgui.hpp>
#include <backend/sdl.hpp>
#include <camera/projection/perspective.hpp>
#include <camera/view/orbit.hpp>
#include <gltf.hpp>
#include <gpu.hpp>
#include <graphic/aa/empty.hpp>
#include <graphic/aa/fxaa.hpp>
#include <graphic/aa/mlaa.hpp>
#include <graphic/aa/smaa.hpp>
#include <graphic/util/smart-texture.hpp>
#include <graphic/util/tool.hpp>
#include <image/algo/mipmap.hpp>
#include <image/compress.hpp>
#include <image/io.hpp>
#include <tiny_gltf.h>
#include <util/asset.hpp>
#include <util/unwrap.hpp>
#include <zip/zip.hpp>

static std::pair<gpu::Graphic_shader, gpu::Graphic_shader> create_shaders(SDL_GPUDevice* device)
{
	auto vertex_shader =
		gpu::Graphic_shader::create(
			device,
			std::as_bytes(shader_asset::simple_vert),
			gpu::Graphic_shader::Stage::Vertex,
			0,
			0,
			0,
			1
		)
		| util::unwrap("创建顶点着色器失败");

	auto fragment_shader =
		gpu::Graphic_shader::create(
			device,
			std::as_bytes(shader_asset::simple_frag),
			gpu::Graphic_shader::Stage::Fragment,
			0,
			0,
			0,
			0
		)
		| util::unwrap("创建片段着色器失败");

	return {std::move(vertex_shader), std::move(fragment_shader)};
}

using Vertex = gltf::Vertex;

static gpu::Graphics_pipeline create_pipeline(
	SDL_GPUDevice* device,
	SDL_Window* window [[maybe_unused]],
	const gpu::Graphic_shader& vertex_shader,
	const gpu::Graphic_shader& fragment_shader
)
{
	SDL_GPURasterizerState rasterizer_state;
	rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
	rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
	rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
	rasterizer_state.depth_bias_clamp = 0;
	rasterizer_state.depth_bias_constant_factor = 0;
	rasterizer_state.depth_bias_slope_factor = 0;
	rasterizer_state.enable_depth_bias = false;
	rasterizer_state.enable_depth_clip = false;

	const auto vertex_attributes = std::to_array<SDL_GPUVertexAttribute>({
		SDL_GPUVertexAttribute{
							   .location = 0,
							   .buffer_slot = 0,
							   .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
							   .offset = offsetof(Vertex, position)
		},
		SDL_GPUVertexAttribute{
							   .location = 1,
							   .buffer_slot = 0,
							   .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
							   .offset = offsetof(Vertex, normal)
		},
		SDL_GPUVertexAttribute{
							   .location = 2,
							   .buffer_slot = 0,
							   .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
							   .offset = offsetof(Vertex, tangent)
		},
		SDL_GPUVertexAttribute{
							   .location = 3,
							   .buffer_slot = 0,
							   .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
							   .offset = offsetof(Vertex, texcoord)
		},
	});

	const auto vertex_buffer_descs = std::to_array<SDL_GPUVertexBufferDescription>({
		{.slot = 0,
		 .pitch = sizeof(Vertex),
		 .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
		 .instance_step_rate = 0},
	});

	const auto primary_color_target = [&] {
		SDL_GPUColorTargetDescription desc;
		desc.format = config::texture::color_texture_format.format;
		desc.blend_state = SDL_GPUColorTargetBlendState{
			.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
			.color_blend_op = SDL_GPU_BLENDOP_ADD,
			.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
			.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
			.color_write_mask = SDL_GPU_COLORCOMPONENT_R
				| SDL_GPU_COLORCOMPONENT_G
				| SDL_GPU_COLORCOMPONENT_B
				| SDL_GPU_COLORCOMPONENT_A,
			.enable_blend = false,
			.enable_color_write_mask = false,
			.padding1 = 0,
			.padding2 = 0
		};
		return desc;
	}();

	const auto depth_stencil_state = gpu::Graphics_pipeline::Depth_stencil_state{
		.format = config::texture::depth_texture_format.format,
		.compare_op = SDL_GPU_COMPAREOP_GREATER,
		.back_stencil_state = {},
		.front_stencil_state = {},
		.compare_mask = 0xFF,
		.write_mask = 0xFF,
		.enable_depth_test = true,
		.enable_depth_write = true,
		.enable_stencil_test = false
	};

	const auto color_target_descs = std::to_array<SDL_GPUColorTargetDescription>({primary_color_target});

	return gpu::Graphics_pipeline::create(
			   device,
			   vertex_shader,
			   fragment_shader,
			   SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
			   SDL_GPU_SAMPLECOUNT_1,
			   rasterizer_state,
			   vertex_attributes,
			   vertex_buffer_descs,
			   color_target_descs,
			   depth_stencil_state
		   )
		| util::unwrap();
}

static gpu::Sampler create_sampler(SDL_GPUDevice* device)
{
	return gpu::Sampler::create(device, gpu::Sampler::Create_info{.max_anisotropy = 4.0}) | util::unwrap();
}

static std::pair<SDL_GPUColorTargetInfo, SDL_GPUDepthStencilTargetInfo> gen_color_target_info(
	SDL_GPUTexture* swapchain,
	SDL_GPUTexture* depth
)
{
	SDL_GPUColorTargetInfo swapchain_target;
	swapchain_target.texture = swapchain;
	swapchain_target.load_op = SDL_GPU_LOADOP_CLEAR;
	swapchain_target.store_op = SDL_GPU_STOREOP_STORE;
	swapchain_target.clear_color = SDL_FColor{.r = 0, .g = 0, .b = 0, .a = 1};
	swapchain_target.resolve_texture = nullptr;
	swapchain_target.cycle = false;
	swapchain_target.mip_level = 0;
	swapchain_target.layer_or_depth_plane = 0;

	SDL_GPUDepthStencilTargetInfo depth_target;
	depth_target.texture = depth;
	depth_target.clear_depth = 0.0f;
	depth_target.load_op = SDL_GPU_LOADOP_CLEAR;
	depth_target.store_op = SDL_GPU_STOREOP_STORE;
	depth_target.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
	depth_target.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
	depth_target.cycle = true;
	depth_target.clear_stencil = 0;

	return {swapchain_target, depth_target};
}

static SDL_GPUColorTargetInfo gen_swapchain_target_info(SDL_GPUTexture* swapchain)
{
	SDL_GPUColorTargetInfo swapchain_target;
	swapchain_target.texture = swapchain;
	swapchain_target.load_op = SDL_GPU_LOADOP_LOAD;
	swapchain_target.store_op = SDL_GPU_STOREOP_STORE;
	swapchain_target.clear_color = SDL_FColor{.r = 0, .g = 0, .b = 0, .a = 1};
	swapchain_target.resolve_texture = nullptr;
	swapchain_target.cycle = false;
	swapchain_target.mip_level = 0;
	swapchain_target.layer_or_depth_plane = 0;

	return swapchain_target;
}

static std::expected<std::vector<std::tuple<gpu::Buffer, gpu::Buffer, uint32_t>>, util::Error>
create_buffer_from_model(SDL_GPUDevice* device, const std::vector<std::byte>& model_data) noexcept
{
	tinygltf::TinyGLTF loader;
	tinygltf::Model model;

	std::string err;
	std::string warn;

	const bool ret = loader.LoadBinaryFromMemory(
		&model,
		&err,
		&warn,
		reinterpret_cast<const unsigned char*>(model_data.data()),
		model_data.size()
	);

	if (!ret) return util::Error(std::format("加载 glTF 模型失败: {}", err));

	std::vector<std::tuple<gpu::Buffer, gpu::Buffer, uint32_t>> buffers;

	for (const auto& mesh : model.meshes)
	{
		for (const auto& primitive : mesh.primitives)
		{
			const auto primitive_cpu = gltf::Primitive::from_tinygltf(model, primitive);
			if (!primitive_cpu) return primitive_cpu.error().propagate("解析 Primitive 失败");

			auto primitive_gpu = gltf::Primitive_gpu::from_primitive(device, *primitive_cpu);
			if (!primitive_gpu) return primitive_gpu.error().propagate("上传 Primitive 到 GPU 失败");

			buffers.emplace_back(
				std::move(primitive_gpu->vertex_buffer),
				std::move(primitive_gpu->index_buffer),
				primitive_gpu->index_count
			);
		}
	}

	return std::move(buffers);
}

#ifdef NDEBUG
constexpr bool is_debug_build = false;
#else
constexpr bool is_debug_build = true;
#endif

int main()
try
{
	/* 初始化 */

	backend::initialize_sdl() | util::unwrap("初始化 SDL 失败");
	const auto sdl_context =
		backend::SDL_context::create(
			config::general::initial_window_width,
			config::general::initial_window_height,
			"图形学大作业技术Demo",
			SDL_WINDOW_RESIZABLE,
			is_debug_build
		)
		| util::unwrap("创建 SDL 上下文失败");

	const auto window = sdl_context->window;
	const auto gpu_device = sdl_context->device;
	const auto swapchain_format = sdl_context->get_swapchain_texture_format();

	backend::initialize_imgui(*sdl_context) | util::unwrap();

	/* 加载模型和贴图 */

	const auto buffers =
		util::get_asset(resource_asset::graphic_asset, "model/WaterBottle.glb")
			.and_then(zip::Decompress(50 * 1048576))
			.and_then([gpu_device](const std::vector<std::byte>& model) {
				return create_buffer_from_model(gpu_device, model);
			})
		| util::unwrap("加载模型失败");

	/* 创建管线 */

	const auto [vertex_shader, fragment_shader] = create_shaders(gpu_device);
	const auto graphics_pipeline = create_pipeline(gpu_device, window, vertex_shader, fragment_shader);
	const auto sampler = create_sampler(gpu_device);

	graphic::aa::FXAA fxaa_processor =
		graphic::aa::FXAA::create(gpu_device, swapchain_format) | util::unwrap("创建 FXAA 处理器失败");
	graphic::aa::MLAA mlaa_processor =
		graphic::aa::MLAA::create(gpu_device, swapchain_format) | util::unwrap("创建 MLAA 处理器失败");
	graphic::aa::SMAA smaa_processor =
		graphic::aa::SMAA::create(gpu_device, swapchain_format) | util::unwrap("创建 SMAA 处理器失败");
	graphic::aa::Empty empty_processor;

	graphic::Smart_texture depth_texture(config::texture::depth_texture_format);
	graphic::Smart_texture color_texture(config::texture::color_texture_format);

	camera::view::Orbit camera_orbit(3, 0, 0, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const camera::view::Orbit::Pan_controller pan_controller{0.5f};
	const camera::view::Orbit::Rotate_controller rotate_controller{
		.azimuth_per_width = glm::radians(360.0f),
		.pitch_per_height = glm::radians(180.0f)
	};

	camera::projection::Perspective camera_projection(glm::radians(45.0f), 0.1f, std::nullopt);

	enum class AA_mode
	{
		None,
		FXAA,
		MLAA,
		SMAA
	};
	AA_mode aa_mode = AA_mode::MLAA;

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

		auto command_buffer = gpu::Command_buffer::acquire_from(gpu_device) | util::unwrap();
		const auto [swapchain_texture, width, height] =
			command_buffer.wait_and_acquire_swapchain_texture(window) | util::unwrap();

		backend::imgui_new_frame();

		if (!ImGui::GetIO().WantCaptureMouse)
		{
			const auto delta = ImGui::GetIO().MouseDelta;
			if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
			{
				rotate_controller.rotate(camera_orbit, {float(width), float(height)}, {delta.x, delta.y});
			}

			if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
			{
				pan_controller.pan(camera_orbit, {float(width), float(height)}, {delta.x, delta.y});
			}

			const auto mouse_wheel = ImGui::GetIO().MouseWheel;
			if (mouse_wheel != 0.0f) camera_orbit.distance *= glm::pow(0.8f, mouse_wheel);
		}

		if (ImGui::Begin("设置", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (ImGui::RadioButton("无抗锯齿", aa_mode == AA_mode::None)) aa_mode = AA_mode::None;
			if (ImGui::RadioButton("FXAA", aa_mode == AA_mode::FXAA)) aa_mode = AA_mode::FXAA;
			if (ImGui::RadioButton("MLAA", aa_mode == AA_mode::MLAA)) aa_mode = AA_mode::MLAA;
			if (ImGui::RadioButton("SMAA", aa_mode == AA_mode::SMAA)) aa_mode = AA_mode::SMAA;
		}
		ImGui::End();

		backend::imgui_upload_data(command_buffer);

		// 渲染
		if (swapchain_texture != nullptr)
		{
			depth_texture.resize(gpu_device, {width, height}) | util::unwrap();
			color_texture.resize(gpu_device, {width, height}) | util::unwrap();

			const auto [color_target, depth_target] = gen_color_target_info(*color_texture, *depth_texture);
			const auto swapchain_color_target = gen_swapchain_target_info(swapchain_texture);

			const glm::mat4 camera_matrix =
				camera_projection.matrix_reverse_z(float(width) / float(height)) * camera_orbit.matrix();
			command_buffer.push_uniform_to_vertex(0, &camera_matrix, sizeof(glm::mat4));

			command_buffer.run_render_pass(
				{&color_target, 1},
				depth_target,
				[&](const gpu::Render_pass& render_pass) {
					render_pass.set_viewport(
						SDL_GPUViewport{
							.x = 0,
							.y = 0,
							.w = float(width),
							.h = float(height),
							.min_depth = 0.0f,
							.max_depth = 1.0f
						}
					);

					render_pass.bind_pipeline(graphics_pipeline);

					for (const auto& [vertex_buffer, index_buffer, index_count] : buffers)
					{
						const SDL_GPUBufferBinding
							vertex_buffer_binding{.buffer = vertex_buffer, .offset = 0};
						const SDL_GPUBufferBinding index_buffer_binding{.buffer = index_buffer, .offset = 0};

						render_pass.bind_vertex_buffers(0, {&vertex_buffer_binding, 1});
						render_pass.bind_index_buffer(index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
						render_pass.draw_indexed(index_count, 0, 1, 0, 0);
					}
				}
			) | util::unwrap();

			graphic::aa::Processor* processor = nullptr;
			switch (aa_mode)
			{
			case AA_mode::None:
				processor = &empty_processor;
				break;
			case AA_mode::FXAA:
				processor = &fxaa_processor;
				break;
			case AA_mode::MLAA:
				processor = &mlaa_processor;
				break;
			case AA_mode::SMAA:
				processor = &smaa_processor;
				break;
			}

			processor->run_antialiasing(
				gpu_device,
				command_buffer,
				*color_texture,
				swapchain_texture,
				{width, height}
			) | util::unwrap();

			command_buffer.run_render_pass(
				{&swapchain_color_target, 1},
				{},
				[&command_buffer](const gpu::Render_pass& render_pass) {
					backend::imgui_draw_to_renderpass(command_buffer, render_pass);
				}
			) | util::unwrap();
		}

		command_buffer.submit() | util::unwrap();
	}

	return EXIT_SUCCESS;
}
catch (const util::Error& e)
{
	std::println(std::cerr, "\033[91m[错误]\033[0m {}", e->front().message);
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