-- Utilities
target("util")
	set_kind("static")
	set_languages("c++23", {public=true})

	add_files("src/**.cpp")
	add_includedirs("include", {public=true})
	add_headerfiles("include/(**.hpp)")
