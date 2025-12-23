-- Camera Controlling Algorithms
target("graphics.camera")
	set_kind("static")
	set_languages("c++23", {public=true})

	add_includedirs("include", {public=true})
	add_headerfiles("include/(**.hpp)")
	add_files("src/**.cpp")
	
	add_packages("glm", {public=true})

	add_deps("graphics.geometry")
