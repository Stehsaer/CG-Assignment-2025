target("wavefront")
	set_kind("static")
	set_languages("c++23", {public=true})

	add_files("src/**.cpp")
	add_includedirs("include", {public=true})
	add_includedirs("detail")
	add_headerfiles("include/(**.hpp)")
	add_headerfiles("detail/(**.hpp)", {install=false})

	add_deps("util", {public=true})
	add_packages("glm", {public=true})