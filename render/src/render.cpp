#include "render.hpp"
#include "backend/imgui.hpp"
#include "render/const-params.hpp"
#include "render/drawdata/gbuffer.hpp"
#include "render/drawdata/shadow.hpp"
#include "render/pass.hpp"
#include "render/pipeline/ambient-light.hpp"
#include "render/pipeline/auto-exposure.hpp"
#include "render/pipeline/directional-light.hpp"
#include "render/pipeline/light.hpp"
#include "render/pipeline/sky-preetham.hpp"
#include "render/pipeline/tonemapping.hpp"
#include "util/error.hpp"

#include <ranges>

namespace render
{
	std::expected<Renderer, util::Error> Renderer::create(const backend::SDL_context& sdl_context) noexcept
	{
		auto pipeline = Pipeline::create(sdl_context);
		if (!pipeline) return pipeline.error().forward("Create pipeline failed");

		auto target = Target::create(sdl_context.device);
		if (!target) return target.error().forward("Create target failed");

		return Renderer(
			std::move(*pipeline),
			std::move(*target),
			graphics::Buffer_pool(sdl_context.device),
			graphics::Transfer_buffer_pool(sdl_context.device)
		);
	}

	std::expected<std::tuple<drawdata::Gbuffer, drawdata::Shadow>, util::Error> Renderer::prepare_drawdata(
		std::span<const gltf::Drawdata> drawdata_list,
		const Params& params
	) noexcept
	{
		auto deferred_resources = drawdata_list
			| std::views::transform(&gltf::Drawdata::deferred_skin_resource)
			| std::views::filter([](const auto& res) { return res != nullptr; });

		const auto camera_matrix = params.camera.proj_matrix * params.camera.view_matrix;

		drawdata::Gbuffer gbuffer_drawdata(camera_matrix, params.camera.eye_position);
		for (const auto& drawdata : drawdata_list) gbuffer_drawdata.append(drawdata);

		drawdata::Shadow shadow_drawdata(
			camera_matrix,
			params.primary_light.direction,
			gbuffer_drawdata.get_min_z(),
			params.shadow.csm_linear_blend
		);
		for (const auto& drawdata : drawdata_list) shadow_drawdata.append(drawdata);

		gbuffer_drawdata.sort();
		shadow_drawdata.sort();

		transfer_buffer_pool.cycle();
		buffer_pool.cycle();

		for (const auto& deferred_data : deferred_resources)
		{
			const auto prepare_result = deferred_data->prepare_gpu_buffers(buffer_pool, transfer_buffer_pool);
			if (!prepare_result) return prepare_result.error().forward("Prepare skinning buffers failed");
		}

		transfer_buffer_pool.gc();
		buffer_pool.gc();

		return std::make_tuple(std::move(gbuffer_drawdata), std::move(shadow_drawdata));
	}

	std::expected<void, util::Error> Renderer::render_gbuffer(
		const gpu::Command_buffer& command_buffer,
		const drawdata::Gbuffer& gbuffer_drawdata,
		const Params& params [[maybe_unused]]
	) const noexcept
	{
		auto gbuffer_pass =
			acquire_gbuffer_pass(command_buffer, target.gbuffer_target, target.light_buffer_target);
		if (!gbuffer_pass) return gbuffer_pass.error().forward("Acquire gbuffer pass failed");
		{
			pipeline.gbuffer_gltf.render(command_buffer, *gbuffer_pass, gbuffer_drawdata);
		}
		gbuffer_pass->end();

		const auto copy_depth_result = pipeline.depth_to_color_copier.copy(
			command_buffer,
			*target.gbuffer_target.depth_texture,
			target.gbuffer_target.depth_value_texture.current()
		);
		if (!copy_depth_result) return copy_depth_result.error().forward("Copy depth to color failed");

		return {};
	}

	std::expected<void, util::Error> Renderer::copy_resources(
		const gpu::Command_buffer& command_buffer,
		std::span<const gltf::Drawdata> drawdata_list
	) const noexcept
	{
		auto deferred_resources = drawdata_list
			| std::views::transform(&gltf::Drawdata::deferred_skin_resource)
			| std::views::filter([](const auto& res) { return res != nullptr; });

		backend::imgui_upload_data(command_buffer);

		const auto copy_deferred_result =
			command_buffer.run_copy_pass([&deferred_resources](const gpu::Copy_pass& copy_pass) {
				for (const auto& deferred_data : deferred_resources)
					deferred_data->upload_gpu_buffers(copy_pass);
			});
		if (!copy_deferred_result)
			return copy_deferred_result.error().forward("Copy deferred skinning buffers failed");

		return {};
	}

	std::expected<void, util::Error> Renderer::render_ao(
		const gpu::Command_buffer& command_buffer,
		const Params& params
	) const noexcept
	{
		const auto camera_matrix = params.camera.proj_matrix * params.camera.view_matrix;

		const pipeline::AO::Params ao_params = {
			.camera_mat_inv = glm::inverse(camera_matrix),
			.view_mat = params.camera.view_matrix,
			.proj_mat_inv = glm::inverse(params.camera.proj_matrix),
			.prev_camera_mat = params.camera.prev_view_proj_matrix,
			.radius = params.ambient.ao_radius,
			.blend_alpha = params.ambient.ao_blend_ratio
		};

		auto ao_pass = acquire_ao_pass(command_buffer, target.ao_target);
		if (!ao_pass) return ao_pass.error().forward("Acquire AO pass failed");
		pipeline.ao.render(command_buffer, *ao_pass, target.ao_target, target.gbuffer_target, ao_params);
		ao_pass->end();
		return {};
	}

	std::expected<void, util::Error> Renderer::render_lighting(
		const gpu::Command_buffer& command_buffer,
		const drawdata::Shadow& shadow_drawdata,
		const Params& params,
		glm::u32vec2 swapchain_size
	) const noexcept
	{
		const pipeline::Directional_light::Params dirlight_params = {
			.camera_matrix_inv = glm::inverse(params.camera.proj_matrix * params.camera.view_matrix),
			.shadow_matrix_level0 = shadow_drawdata.get_vp_matrix(0),
			.shadow_matrix_level1 = shadow_drawdata.get_vp_matrix(1),
			.shadow_matrix_level2 = shadow_drawdata.get_vp_matrix(2),
			.eye_position = params.camera.eye_position,
			.light_direction = params.primary_light.direction,
			.light_color = params.primary_light.intensity / REF_LUMINANCE
		};

		const pipeline::Ambient_light::Param ambient_light_params = {
			.ambient_intensity = params.ambient.intensity / REF_LUMINANCE,
			.ao_strength = params.ambient.ao_strength
		};

		const pipeline::Sky_preetham::Params sky_params = {
			.camera_mat_inv = glm::inverse(params.camera.proj_matrix * params.camera.view_matrix),
			.screen_size = swapchain_size,
			.eye_position = params.camera.eye_position,
			.sun_direction = params.primary_light.direction,
			.sun_intensity = params.primary_light.intensity * params.sky.brightness_mult / REF_LUMINANCE,
			.turbidity = params.sky.turbidity
		};

		auto lighting_pass =
			acquire_lighting_pass(command_buffer, target.light_buffer_target, target.gbuffer_target);
		if (!lighting_pass) return lighting_pass.error().forward("Acquire lighting pass failed");

		pipeline.directional_light.render(
			command_buffer,
			*lighting_pass,
			target.gbuffer_target,
			target.shadow_target,
			dirlight_params
		);

		pipeline.ambient_light.render(
			command_buffer,
			*lighting_pass,
			target.gbuffer_target,
			target.ao_target,
			ambient_light_params
		);

		pipeline.sky_preetham.render(command_buffer, *lighting_pass, sky_params);

		lighting_pass->end();
		return {};
	}

	std::expected<void, util::Error> Renderer::render_lights(
		const gpu::Command_buffer& command_buffer,
		const target::Gbuffer& gbuffer_target,
		const target::Light_buffer& light_buffer_target,
		std::span<const drawdata::Light> lights,
		const Params& params,
		glm::u32vec2 swapchain_size
	) const noexcept
	{
		const pipeline::Light::Param point_light_param = {
			.camera_view_projection = params.camera.proj_matrix * params.camera.view_matrix,
			.eye_position = params.camera.eye_position,
			.screen_size = swapchain_size
		};

		const auto render_result =
			pipeline.point_light
				.render(command_buffer, gbuffer_target, light_buffer_target, lights, point_light_param);
		if (!render_result) return render_result.error().forward("Render primary point light failed");

		return {};
	}

	std::expected<void, util::Error> Renderer::render_ssgi(
		const gpu::Command_buffer& command_buffer,
		const drawdata::Gbuffer& gbuffer_drawdata,
		const Params& params,
		glm::u32vec2 swapchain_size
	) const noexcept
	{
		const pipeline::SSGI::Param ssgi_params = {
			.proj_mat = params.camera.proj_matrix,
			.view_mat = params.camera.view_matrix,
			.prev_view_proj_mat = params.camera.prev_view_proj_matrix,
			.max_scene_distance = gbuffer_drawdata.get_max_distance(),
			.distance_attenuation = 0.0,
			.blend_factor = 0.0125
		};

		const auto ssgi_result = pipeline.ssgi.render(
			command_buffer,
			target.light_buffer_target,
			target.gbuffer_target,
			target.ssgi_target,
			ssgi_params,
			swapchain_size
		);
		if (!ssgi_result) return ssgi_result.error().forward("Render SSGI failed");

		return {};
	}

	std::expected<void, util::Error> Renderer::compute_auto_exposure(
		const gpu::Command_buffer& command_buffer,
		glm::u32vec2 swapchain_size
	) const noexcept
	{
		const pipeline::Auto_exposure::Params auto_exposure_params = {
			.min_luminance = 1e-2,
			.max_luminance = 500,
			.eye_adaptation_rate = 1.5,
			.delta_time = ImGui::GetIO().DeltaTime
		};

		const auto auto_exposure_result = pipeline.auto_exposure.compute(
			command_buffer,
			target.auto_exposure_target,
			target.light_buffer_target,
			auto_exposure_params,
			swapchain_size
		);
		if (!auto_exposure_result)
			return auto_exposure_result.error().forward("Compute auto exposure failed");

		return {};
	}

	std::expected<void, util::Error> Renderer::render_bloom(
		const gpu::Command_buffer& command_buffer,
		const Params& params,
		glm::u32vec2 swapchain_size
	) const noexcept
	{
		const pipeline::Bloom::Param bloom_render_params =
			{.start_threshold = 2.0, .end_threshold = 10.0, .attenuation = params.bloom.bloom_attenuation};
		const auto bloom_result = pipeline.bloom.render(
			command_buffer,
			target.light_buffer_target,
			target.bloom_target,
			target.auto_exposure_target,
			bloom_render_params,
			swapchain_size
		);
		if (!bloom_result) return bloom_result.error().forward("Render bloom failed");

		return {};
	}

	std::expected<void, util::Error> Renderer::render_composite(
		SDL_GPUDevice* device,
		const gpu::Command_buffer& command_buffer,
		const Params& params,
		glm::u32vec2 swapchain_size,
		SDL_GPUTexture* swapchain
	) noexcept
	{
		const pipeline::Tonemapping::Param tonemapping_params = {
			.bloom_strength = params.bloom.bloom_strength,
			.use_bloom_mask = params.function_mask.use_bloom_mask
		};

		const auto tonemapping_result = pipeline.tonemapping.render(
			command_buffer,
			target.light_buffer_target,
			target.auto_exposure_target,
			target.bloom_target,
			*target.composite_target.composite_texture,
			tonemapping_params
		);
		if (!tonemapping_result) return tonemapping_result.error().forward("Render tonemapping failed");

		const auto aa_result = pipeline.aa_module.run(
			device,
			command_buffer,
			*target.composite_target.composite_texture,
			swapchain,
			swapchain_size,
			params.aa_mode
		);
		if (!aa_result) return aa_result.error().forward("Run antialiasing failed");

		return {};
	}

	std::expected<void, util::Error> Renderer::render_imgui(
		const gpu::Command_buffer& command_buffer,
		SDL_GPUTexture* swapchain
	) const noexcept
	{
		auto swapchain_pass = acquire_swapchain_pass(command_buffer, swapchain, false);
		if (!swapchain_pass) return swapchain_pass.error().forward("Acquire swapchain pass failed");
		{
			backend::imgui_draw_to_renderpass(command_buffer, *swapchain_pass);
		}
		swapchain_pass->end();

		return {};
	}

	std::expected<void, util::Error> Renderer::render(
		const backend::SDL_context& sdl_context,
		Drawdata drawdata,
		const Params& params
	) noexcept
	{
		/* Preparation */

		auto prepare_result = prepare_drawdata(drawdata.models, params);
		if (!prepare_result) return prepare_result.error().forward("Prepare drawdata failed");
		const auto [gbuffer_drawdata, shadow_drawdata] = std::move(*prepare_result);

		/* Acquire Command Buffer */

		auto command_buffer = gpu::Command_buffer::acquire_from(sdl_context.device);
		if (!command_buffer) return command_buffer.error().forward("Acquire command buffer failed");

		const auto swapchain_acquire_result =
			command_buffer->wait_and_acquire_swapchain_texture(sdl_context.window);
		if (!swapchain_acquire_result)
			return swapchain_acquire_result.error().forward("Acquire swapchain texture failed");

		const auto [swapchain_texture, swapchain_width, swapchain_height] = *swapchain_acquire_result;
		const glm::u32vec2 swapchain_size = {swapchain_width, swapchain_height};

		if (swapchain_texture == nullptr)
		{
			if (const auto result = command_buffer->submit(); !result)
				return result.error().forward("Submit command buffer failed");

			return {};
		}

		/* Resize */

		if (const auto result = target.resize_or_cycle(sdl_context.device, swapchain_size); !result)
			return result.error().forward("Resize or cycle render targets failed");

		/* Copy */

		const auto copy_result = copy_resources(*command_buffer, drawdata.models);
		if (!copy_result) return copy_result.error().forward("Copy resources failed");

		/* Render */

		const auto gbuffer_result = render_gbuffer(*command_buffer, gbuffer_drawdata, params);
		if (!gbuffer_result) return gbuffer_result.error().forward("Render G-buffer failed");

		const auto hiz_result =
			pipeline.hiz_generator.generate(*command_buffer, target.gbuffer_target, swapchain_size);
		if (!hiz_result) return hiz_result.error().forward("Generate Hi-Z failed");

		const auto shadow_result =
			pipeline.shadow_gltf.render(*command_buffer, target.shadow_target, shadow_drawdata);
		if (!shadow_result) return shadow_result.error().forward("Render shadow failed");

		const auto ao_result = render_ao(*command_buffer, params);
		if (!ao_result) return ao_result.error().forward("Render AO failed");

		const auto lighting_result =
			render_lighting(*command_buffer, shadow_drawdata, params, swapchain_size);
		if (!lighting_result) return lighting_result.error().forward("Render lighting failed");

		const auto primary_lights_result = render_lights(
			*command_buffer,
			target.gbuffer_target,
			target.light_buffer_target,
			drawdata.lights,
			params,
			swapchain_size
		);
		if (!primary_lights_result)
			return primary_lights_result.error().forward("Render primary lights failed");

		if (params.function_mask.ssgi)
		{
			const auto ssgi_result = render_ssgi(*command_buffer, gbuffer_drawdata, params, swapchain_size);
			if (!ssgi_result) return ssgi_result.error().forward("Render SSGI failed");
		}

		const auto auto_exposure_result = compute_auto_exposure(*command_buffer, swapchain_size);
		if (!auto_exposure_result)
			return auto_exposure_result.error().forward("Compute auto exposure failed");

		const auto bloom_result = render_bloom(*command_buffer, params, swapchain_size);
		if (!bloom_result) return bloom_result.error().forward("Render bloom failed");

		const auto composite_result =
			render_composite(sdl_context.device, *command_buffer, params, swapchain_size, swapchain_texture);
		if (!composite_result) return composite_result.error().forward("Render composite failed");

		const auto imgui_result = render_imgui(*command_buffer, swapchain_texture);
		if (!imgui_result) return imgui_result.error().forward("Render ImGui failed");

		const auto submit_result = command_buffer->submit();
		if (!submit_result) return submit_result.error().forward("Submit command buffer failed");

		return {};
	}
}