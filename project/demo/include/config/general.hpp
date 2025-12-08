#pragma once

namespace config::general
{
	constexpr int initial_window_width = 800;
	constexpr int initial_window_height = 600;
	constexpr const char* window_title = "图形学大作业技术验证";

#ifdef NDEBUG
	constexpr bool enable_debug_layer = false;
#else
	constexpr bool enable_debug_layer = true;
#endif
}  // namespace config::general