-- Rules and Policies
add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.asan")
set_policy("build.warning", true)
set_policy("build.intermediate_directory", false)
set_policy("run.autobuild", true)

-- Compilation Settings
set_encodings("utf-8")
set_warnings("all", "pedantic", "extra")
set_languages("c++23")
add_vectorexts("sse", "sse2", "avx", "avx2")

-- Additional Links
if is_plat("linux") then
	add_syslinks("atomic")
end

-- Additional Defs
add_defines("GLM_FORCE_DEPTH_ZERO_TO_ONE", "GLM_ENABLE_EXPERIMENTAL", "GLM_FORCE_INTRINSICS")
add_defines("TINYGLTF_NOEXCEPTION")

-- Packages
add_requires(
	"libsdl3 main",
	"glm 1.0.2",
	"gzip-hpp v0.1.0",
	"stb 2025.03.14",
	"tinygltf v2.9.6",
	"meshoptimizer v0.25",
	"paul_thread_pool 0.7.0",
	"vulkan-headers 1.4.309+0"
)
add_requires(
	"imgui v1.92.1-docking", 
	{configs={sdl3=true, sdl3_gpu=true, wchar32=true}}
)
add_requireconfs("imgui.libsdl3", {override=true, version="main"})

-- Rules, Tasks and subprojects
includes("xmake/rule", "xmake/task/*.lua")
includes("project", "lib", "render")
