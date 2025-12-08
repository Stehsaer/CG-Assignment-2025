-- Image-related algorithms
target("image.algo")
	set_kind("static")
	set_languages("c++23", {public=true})

	add_includedirs("include", {public=true})
	add_headerfiles("include/(**.hpp)")
	add_files("src/**.cpp")

	add_deps("image.repr", "util", {public=true})
