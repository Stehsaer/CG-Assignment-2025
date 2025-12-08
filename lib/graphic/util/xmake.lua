-- Graphic Utilities
target("graphic.util")
	set_kind("static")
	set_languages("c++23", {public=true})

	add_rules("asset.shader", {debug = is_mode("debug")})
	add_files("shader/*")

	add_files("src/**.cpp")
	add_headerfiles("include/(**.hpp)")
	add_includedirs("include", {public=true})

	add_deps("gpu", "image.repr", {public=true})
	add_packages("glm", {public=true})