-- Antialiasing Algorithms
target("graphic.aa")
	set_kind("static")
	set_languages("c++23")

	add_rules("asset.shader", {debug = is_mode("debug")})
	add_files("shader/*")

	add_files("src/**.cpp")
	add_includedirs("include", {public=true})
	add_headerfiles("include/(**.hpp)")
	
	add_deps("graphic.util", {public=true})