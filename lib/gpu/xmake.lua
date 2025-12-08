-- SDL GPU Wrapping
target("gpu")
	set_kind("static")
	set_languages("c++23", {public=true})

	add_packages("libsdl3", {public=true}) -- SDL3

	add_files("src/**.cpp")
	add_headerfiles("include/(**.hpp)")
	add_includedirs("include", {public=true})

	add_deps("util", {public=true})
