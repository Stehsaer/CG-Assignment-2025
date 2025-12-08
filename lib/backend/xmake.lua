-- Backends, including SDL Window, SDL Device and ImGui integration
target("backend")
	set_kind("static")
	set_languages("c++23", {public=true})

	add_packages("libsdl3", "glm", "imgui", {public=true})

	add_files("src/*.cpp")
	add_includedirs("include", {public=true})
	add_headerfiles("include/(**.hpp)")

	add_deps("gpu", "zip")

	add_rules("asset.pack")
	add_files("asset/*.pack-desc")

