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

option("scene")
    set_showmenu(true)
    set_description("Scene model data path")
    set_default("")

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
    add_files("scene/scene.pack-desc")

    before_prepare(function (target)
        import("core.project.config")

        local scene_path = config.get("scene")
        local output_path = os.scriptdir() .. "/scene/scene.glb"

        if not scene_path or not os.exists(scene_path) or not os.isfile(scene_path) then
            raise("Scene data file not found: %s", scene_path or "nil")
        end

        os.cp(scene_path, output_path, {copy_if_different=true})
    end)
