package("imgui-knob")
    set_homepage("https://github.com/altschuler/imgui-knobs")
    set_description("This is a port/adaptation of imgui-rs-knobs, for C++.")
    set_license("MIT")

    add_urls("https://github.com/altschuler/imgui-knobs.git")

    add_deps("imgui")

    on_install(function (package)
        io.writefile("xmake.lua", [[
            add_rules("mode.release")
            set_languages("c++11")
            add_requires("imgui")
            target("imgui-color-text-edit")
                set_kind("$(kind)")

                add_packages("imgui")

                add_files("*.cpp")
                add_headerfiles("*.h")

                if is_plat("windows") and is_kind("shared") then
                    add_rules("utils.symbols.export_all", {export_classes = true})
                end
        ]])
        import("package.tools.xmake").install(package)
    end)
package_end()

add_requires("imgui-knob")

target("homework3")
	set_kind("binary")
	add_rules("compile.slang")

	add_files("src/**.cpp")
	add_includedirs("include")
	add_files("shader/**.slang")

	add_deps("lib::backend", "lib::gpu", "lib::util", "lib::graphics.util", "lib::graphics.camera")
	add_packages("imgui-knob")