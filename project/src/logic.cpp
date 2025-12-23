#include "logic.hpp"
#include "backend/sdl.hpp"
#include "render/param.hpp"

#include <imgui.h>
#include <ranges>

void Logic::light_control_ui() noexcept
{
	ImGui::SeparatorText("光照");

	ImGui::SliderAngle("方位角", &light_azimuth, -180.0, 180.0);
	ImGui::SliderAngle("高度角", &light_pitch, -89.0, 89.0);
	ImGui::ColorEdit3("颜色", &light_color.r);
	ImGui::SliderFloat("强度", &light_intensity, 0.1f, 50.0f, "%.1f", ImGuiSliderFlags_Logarithmic);
	ImGui::SliderFloat("大气浑浊度", &turbidity, 2.0f, 5.0f);
	ImGui::SliderFloat("天空亮度倍率", &sky_brightness_mult, 0.0f, 2.0f);

	ImGui::Separator();

	ambient_lighting.control_ui();

	ImGui::Separator();

	ImGui::SliderFloat("CSM 线性混合", &csm_linear_blend, 0.0f, 1.0f);

	ImGui::Separator();

	ImGui::SliderFloat("Bloom 衰减", &bloom_attenuation, 0.0f, 5.0f);
	ImGui::SliderFloat("Bloom 强度", &bloom_strength, 0.001f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
	ImGui::Checkbox("脏镜头效果", &use_bloom_mask);
}

void Logic::antialias_control_ui() noexcept
{
	using namespace render;

	ImGui::SeparatorText("抗锯齿");

	if (ImGui::RadioButton("无抗锯齿", aa_mode == Antialias_mode::None)) aa_mode = Antialias_mode::None;
	if (ImGui::RadioButton("FXAA", aa_mode == Antialias_mode::FXAA)) aa_mode = Antialias_mode::FXAA;
	if (ImGui::RadioButton("MLAA", aa_mode == Antialias_mode::MLAA)) aa_mode = Antialias_mode::MLAA;
	if (ImGui::RadioButton("SMAA", aa_mode == Antialias_mode::SMAA)) aa_mode = Antialias_mode::SMAA;
}

void Logic::statistic_display_ui() const noexcept
{
	const auto& io = ImGui::GetIO();

	ImGui::SeparatorText("统计");

	ImGui::Text("Res: (%.0f, %.0f)", io.DisplaySize.x, io.DisplaySize.y);
	ImGui::Text("FPS: %.1f FPS", io.Framerate);
}

void Logic::debug_control_ui() noexcept
{
	ImGui::SeparatorText("调试");

	ImGui::DragFloat("时间", &time, 0.01f, 0.0f, 1000.0f);

	if (ImGui::Button("播放/暂停")) time_run = !time_run;
	if (ImGui::Button("重置时间")) time = 0.0f;

	if (time_run) time += ImGui::GetIO().DeltaTime;
}

std::tuple<render::Params, std::vector<gltf::Drawdata>> Logic::logic(
	const backend::SDL_context& context,
	const gltf::Model& model
) noexcept
{
	camera_control.update(context);

	if (ImGui::Begin("设置", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		light_control_ui();
		antialias_control_ui();
		statistic_display_ui();
		debug_control_ui();
	}
	ImGui::End();

	const auto light_direction = glm::vec3(
		glm::cos(light_pitch) * glm::sin(light_azimuth),
		glm::sin(light_pitch),
		glm::cos(light_pitch) * glm::cos(light_azimuth)
	);

	std::vector<gltf::Animation_key> animation_keys;
	for (const auto idx : std::views::iota(0zu, model.get_animations().size()))
		animation_keys.push_back(gltf::Animation_key{.animation = uint32_t(idx), .time = time});

	std::vector<gltf::Drawdata> drawdata_list;
	drawdata_list.emplace_back(model.generate_drawdata(glm::mat4(1.0f), animation_keys));

	const render::Primary_light_params primary_light{
		.direction = light_direction,
		.intensity = light_color * light_intensity,
	};

	const render::Bloom_params bloom_params{
		.bloom_attenuation = bloom_attenuation,
		.bloom_strength = bloom_strength,
	};

	const render::Shadow_params shadow_params{
		.csm_linear_blend = csm_linear_blend,
	};

	const render::Sky_params sky_params{
		.turbidity = turbidity,
		.brightness_mult = sky_brightness_mult,
	};

	const render::Params params{
		.aa_mode = aa_mode,
		.camera = camera_control.get_matrices(),
		.primary_light = primary_light,
		.ambient = ambient_lighting.get_params(),
		.bloom = bloom_params,
		.shadow = shadow_params,
		.sky = sky_params,
		.function_mask = {.use_bloom_mask = use_bloom_mask}
	};

	return std::make_tuple(params, std::move(drawdata_list));
}
