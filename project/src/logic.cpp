#include "logic.hpp"

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
}

void Logic::antialias_control_ui() noexcept
{
	using Mode = renderer::Antialias::Mode;

	ImGui::SeparatorText("抗锯齿");

	if (ImGui::RadioButton("无抗锯齿", aa_mode == Mode::None)) aa_mode = Mode::None;
	if (ImGui::RadioButton("FXAA", aa_mode == Mode::FXAA)) aa_mode = Mode::FXAA;
	if (ImGui::RadioButton("MLAA", aa_mode == Mode::MLAA)) aa_mode = Mode::MLAA;
	if (ImGui::RadioButton("SMAA", aa_mode == Mode::SMAA)) aa_mode = Mode::SMAA;
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

	if (ImGui::RadioButton("无调试", debug_mode == Debug_mode::No_debug)) debug_mode = Debug_mode::No_debug;
	if (ImGui::RadioButton("显示 AO", debug_mode == Debug_mode::Show_AO)) debug_mode = Debug_mode::Show_AO;
	if (ImGui::RadioButton("显示 SSGI 追踪", debug_mode == Debug_mode::Show_SSGI_trace))
		debug_mode = Debug_mode::Show_SSGI_trace;

	ImGui::DragFloat("时间", &time, 0.01f, 0.0f, 1000.0f);

	if (ImGui::Button("播放/暂停")) time_run = !time_run;
	if (ImGui::Button("重置时间")) time = 0.0f;

	if (time_run) time += ImGui::GetIO().DeltaTime;
}

Logic::Result Logic::logic(const gltf::Model& model) noexcept
{
	camera_control.update();

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

	return {
		.aa_mode = aa_mode,
		.ambient_lighting = ambient_lighting,
		.camera = camera_control.get_matrices(),
		.light_direction = light_direction,
		.light_color = light_color * light_intensity,
		.turbidity = turbidity,
		.sky_brightness_mult = sky_brightness_mult,
		.bloom_attenuation = bloom_attenuation,
		.bloom_strength = bloom_strength,
		.debug_mode = debug_mode,
		.csm_linear_blend = csm_linear_blend,
		.animation_keys = std::move(animation_keys)
	};
}
