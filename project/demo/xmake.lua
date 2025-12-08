target("demo")
    set_kind("binary")
    set_languages("c++23") -- C++23标准

    -- 第三方库依赖
    add_packages("libsdl3", "glm", "imgui", "rygs-dxtc")

    -- 自定义规则
    add_rules("asset.shader", {debug = is_mode("debug"), includedir = "shader/include"})
    add_rules("asset.pack")

    -- 添加代码
    add_files("src/**.cpp")
    add_includedirs("include")
    add_headerfiles("include/(**.hpp)")

    -- 添加着色器
    add_files("shader/**")
    remove_files("shader/**.glsl") -- 排除着色器头文件

    -- 添加资源包描述
    add_files("asset/*.pack-desc")

    -- 子功能库
    add_deps(
        "lib::gpu", 
        "lib::gltf-gpu", 
        "lib::zip", 
        "lib::util", 
        "lib::backend", 
        "lib::camera",
        "lib::image.io",
        "lib::image.compress",
        "lib::image.algo",
        "lib::graphic.util",
        "lib::graphic.aa"
    )
