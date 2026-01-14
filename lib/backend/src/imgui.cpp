#include "backend/imgui.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>
#include <implot.h>

#include "asset/imgui-asset.hpp"
#include "zip/zip.hpp"

namespace backend
{
	static std::expected<void, util::Error> load_imgui_font() noexcept
	{
		auto& io = ImGui::GetIO();
		constexpr float font_size = 16.0f;

		ImFontConfig config;
		config.FontDataOwnedByAtlas = false;

		auto display_font = zip::decompress(resource_asset::imgui_asset.at("display.ttf"));
		if (!display_font) return display_font.error().forward("Decompress display font failed");

		auto symbol_font = zip::decompress(resource_asset::imgui_asset.at("symbol.ttf"));
		if (!symbol_font) return symbol_font.error().forward("Decompress symbol font failed");

		const auto display_font_add_result = io.Fonts->AddFontFromMemoryTTF(
			display_font->data(),
			static_cast<int>(display_font->size()),
			font_size,
			&config
		);

		config.MergeMode = true;

		const auto symbol_font_add_result = io.Fonts->AddFontFromMemoryTTF(
			symbol_font->data(),
			static_cast<int>(symbol_font->size()),
			font_size,
			&config
		);

		if (display_font_add_result == nullptr || symbol_font_add_result == nullptr)
			return util::Error("Add IMGUI font failed");

		return {};
	}

	static void set_imgui_style() noexcept
	{
		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();

		style.WindowRounding = 12.0f;

		style.ChildRounding = 6.0f;
		style.FrameRounding = 6.0f;
		style.PopupRounding = 6.0f;
		style.ScrollbarRounding = 6.0f;
		style.GrabRounding = 6.0f;
		style.TabRounding = 6.0f;

		style.AntiAliasedLines = true;
		style.AntiAliasedFill = true;

		style.Colors[ImGuiCol_PlotHistogram] = style.Colors[ImGuiCol_ButtonHovered];
		style.Colors[ImGuiCol_PlotLines] = style.Colors[ImGuiCol_ButtonHovered];
		style.Colors[ImGuiCol_PlotHistogramHovered] = style.Colors[ImGuiCol_ButtonHovered];
		style.Colors[ImGuiCol_PlotLinesHovered] = style.Colors[ImGuiCol_ButtonHovered];
		style.Colors[ImGuiCol_ModalWindowDimBg] = {0.1, 0.1, 0.1, 0.5};

		style.WindowTitleAlign = {0.5f, 0.5f};
	}

	std::expected<void, util::Error> initialize_imgui(const SDLcontext& sdl_context) noexcept
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImPlot::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		ImGui_ImplSDLGPU3_InitInfo init_info = {};
		init_info.Device = sdl_context.device;
		init_info.ColorTargetFormat = sdl_context.get_swapchain_texture_format();
		init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;

		if (!ImGui_ImplSDL3_InitForSDLGPU(sdl_context.window))
			return util::Error("Initialize IMGUI SDL3 backend failed");
		if (!ImGui_ImplSDLGPU3_Init(&init_info))
			return util::Error("Initialize IMGUI SDL-GPU3 backend failed");

		set_imgui_style();
		if (const auto load_font_result = load_imgui_font(); !load_font_result)
			return load_font_result.error().forward("Load IMGUI font failed");

		ImGuiStyle& style = ImGui::GetStyle();
		const auto main_scale = sdl_context.get_window_scale();
		style.ScaleAllSizes(main_scale);
		style.FontScaleDpi = main_scale;

		return {};
	}

	void destroy_imgui() noexcept
	{
		ImGui_ImplSDLGPU3_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImPlot::DestroyContext();
		ImGui::DestroyContext();
	}

	void imgui_handle_event(const SDL_Event* event) noexcept
	{
		ImGui_ImplSDL3_ProcessEvent(event);
	}

	void imgui_new_frame() noexcept
	{
		ImGui_ImplSDLGPU3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();
	}

	void imgui_upload_data(const gpu::CommandBuffer& command_buffer) noexcept
	{
		ImGui::Render();
		ImGui_ImplSDLGPU3_PrepareDrawData(ImGui::GetDrawData(), command_buffer);
	}

	void imgui_draw_to_renderpass(
		const gpu::CommandBuffer& command_buffer,
		const gpu::RenderPass& render_pass
	) noexcept
	{
		ImGui_ImplSDLGPU3_RenderDrawData(ImGui::GetDrawData(), command_buffer, render_pass);
	}
}