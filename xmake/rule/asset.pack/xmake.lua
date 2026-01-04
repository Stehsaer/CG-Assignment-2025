rule("asset.pack")
	set_extensions(".pack-desc")

	on_load(function (target)
		import("funcs").load_rule(target)
	end)

	on_prepare_file(function (target, source_path, opt)
		import("funcs").prepare_file(target, source_path, opt)
	end)

	on_build_file(function (target, source_path, opt)
		import("funcs").build_file(target, source_path, opt)
	end)