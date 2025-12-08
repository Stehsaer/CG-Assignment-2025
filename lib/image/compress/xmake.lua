package("bc7enc")

	set_urls("https://github.com/Stehsaer/bc7enc_rdo/archive/104269f1ef9e3f78c523d83a7d88d4c7eeadb367.zip")

	add_versions("latest", "caa655ce53a45528f32356e713a4381e5e59a76cccfd59ebb25a76d7cedca241")

	on_install(function (package)
        import("package.tools.xmake").install(package)
	end)

package_end()

package("stb_dxt")

	set_urls("https://github.com/Cyan4973/RygsDXTc.git")

	on_install(function(package)
		io.writefile("xmake.lua", [[
			add_rules("mode.release")
			set_languages("c11")
			add_vectorexts("sse", "sse2", "avx", "avx2")
			target("stb_dxt")
				set_kind("static")
				add_files("stb_dxt.cpp")
				add_headerfiles("stb_dxt.h")
		]])
        import("package.tools.xmake").install(package)
	end)

package_end()

add_requires("bc7enc", "stb_dxt")

-- Image compressing tool
target("image.compress")
	set_kind("static")
	set_languages("c++23", {public=true})

	add_includedirs("include", {public=true})
	add_headerfiles("include/(**.hpp)")
	add_files("src/**.cpp")

	add_packages("bc7enc", "stb_dxt")
	add_deps("image.repr", {public=true})