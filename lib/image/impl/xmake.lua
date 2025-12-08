target("image.impl")
	set_kind("static")
	add_files("impl.cpp")
	add_packages("stb", {public=true})