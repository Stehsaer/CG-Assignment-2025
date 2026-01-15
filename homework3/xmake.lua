target("homework3")
	set_kind("binary")
	add_rules("compile.slang")

	add_files("src/**.cpp")
	add_includedirs("include")
	add_files("shader/**.slang")

	add_deps("lib::backend", "lib::gpu", "lib::util", "lib::graphics.util")