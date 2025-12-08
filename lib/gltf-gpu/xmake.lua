-- GLTF GPU Loader
target("gltf-gpu")
	set_kind("static")
	set_languages("c++23", {public=true})

	add_packages(
		"glm", 
		"stb", 
		"libsdl3", 
		"tinygltf",
		"meshoptimizer",
		{public=true}
	) 

	add_files("src/**.cpp")
	add_includedirs("include", {public=true})
	add_headerfiles("include/(**.hpp)")
	
	add_deps(
		"util", 
		"image.algo", 
		"image.compress", 
		"gpu", 
		"graphic.util",
		{public=true}
	)
