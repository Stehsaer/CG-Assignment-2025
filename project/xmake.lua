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

target("main")
    set_kind("binary")
    set_languages("c++23") -- C++23标准

    add_rules("asset.pack")

    add_files("src/**.cpp")
    add_includedirs("include", {public=true})
    add_headerfiles("include/(**.hpp)")
    add_files("asset/*.pack-desc")

    add_deps("render", "lib::wavefront")
    add_packages("imgui-knob")

    add_rules("asset.pack")
    add_files("asset/*.pack-desc")
