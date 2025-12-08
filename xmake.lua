add_rules("mode.debug", "mode.release", "mode.releasedbg")
set_policy("build.warning", true)
set_policy("build.intermediate_directory", false)
set_policy("run.autobuild", true)

set_encodings("utf-8")
set_warnings("all", "pedantic")
set_languages("c++23")

add_cxflags("-ffunction-sections", "-fdata-sections")
add_vectorexts("sse", "sse2", "avx", "avx2")

if is_plat("linux") then
	add_syslinks("atomic")
end

includes("xmake/rule", "xmake/task/*.lua")
add_defines("GLM_FORCE_DEPTH_ZERO_TO_ONE", "GLM_ENABLE_EXPERIMENTAL")
add_defines("TINYGLTF_NOEXCEPTION")

add_requires(
	"libsdl3 3.2.22",
	"glm 1.0.1",
	"gzip-hpp v0.1.0",
	"stb 2025.03.14",
	"tinygltf v2.9.6",
	"meshoptimizer v0.25",
	"paul_thread_pool 0.7.0"
)
add_requires("imgui v1.92.1-docking", {configs={sdl3=true, sdl3_gpu=true, wchar32=true}})

includes("project", "lib")
