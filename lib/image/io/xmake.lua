-- Image IO
target("image.io")
	set_kind("static")
	set_languages("c++23", {public=true})
	
	add_includedirs("include", {public=true})
	add_headerfiles("include/(**.hpp)")
	add_files("src/**.cpp")

	add_deps("util", "image.repr", "image.impl", {public=true})
	add_packages("glm", "stb", {public=true})
