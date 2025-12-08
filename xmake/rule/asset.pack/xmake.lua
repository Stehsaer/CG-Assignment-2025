function _load(target)
	import("get_path")
	local paths = get_path(target)
	local public_include = target:extraconf("rules", "asset.pack", "public_include") or false
	target:add("includedirs", paths.header_root, {public=public_include})
end

function _prepare(target, source_path, opt)
	import("core.base.json")
	import("core.project.depend")
	import("get_path")
	import("lib.detect.find_tool")
	import("utils.progress")

	local python = find_tool("python3") or find_tool("python")
	assert(python, "Python tool not available")
	
	local paths = get_path(target)

	-- Create output directories

	if not os.exists(paths.header) then
		os.mkdir(paths.header)
	end
	if not os.exists(paths.cpp) then
		os.mkdir(paths.cpp)
	end

	-- Find directories

	local source_name = path.filename(source_path)
	local source_basepath = path.directory(source_path)

	local cpp_output_path = path.join(paths.cpp, source_name .. ".cpp")
	local object_output_path = target:objectfile(cpp_output_path)
	local header_output_path = path.join(paths.header, path.basename(source_path) .. ".hpp")

	local namespace = "resource_asset"
	local varname = string.gsub(path.basename(source_path), "[%.%-]", "_")

	-- Read description file and get dependency list

	local glob_list = json.loadfile(source_path)
	local file_list = {}

	for _, pattern in ipairs(glob_list) do
		for _, f in ipairs(os.files(path.join(source_basepath, pattern))) do
			file_list[path.relative(f, os.projectdir())] = true
		end
	end

	local dep_list = {}
	for f, _ in pairs(file_list) do
		table.insert(dep_list, f)
	end
	table.insert(dep_list, source_path)

	-- Pack header file

	depend.on_changed(function ()
		os.vrunv(python.program, {
			paths.script,
			"--input", source_path, 
			"--output", header_output_path, 
			"--type", "header", 
			"--namespace", namespace, 
			"--varname", varname,
			"--root", path.directory(source_path)
		})
	end, {
		files = dep_list, 
		lastmtime = os.mtime(header_output_path),
		changed = target:is_rebuilt() or not os.exists(header_output_path)
	})

end


function _build(target, source_path, opt)
	import("core.base.json")
	import("core.project.config")
	import("core.project.depend")
	import("core.tool.compiler")
	import("lib.detect.find_tool")
	import("utils.progress")
	import("get_path")

	local paths = get_path(target)

	local python = find_tool("python3") or find_tool("python")
	assert(python, "Python tool not available")

	-- Find directories

	local source_name = path.filename(source_path)
	local source_basepath = path.directory(source_path)

	local cpp_output_path = path.join(paths.cpp, source_name .. ".cpp")
	local object_output_path = target:objectfile(cpp_output_path)
	local header_output_path = path.join(paths.header, path.basename(source_path) .. ".hpp")

	local namespace = "resource_asset"
	local varname = string.gsub(path.basename(source_path), "[%.%-]", "_")

	-- Read description file and get dependency list

	local glob_list = json.loadfile(source_path)
	local file_list = {}

	for _, pattern in ipairs(glob_list) do
		for _, f in ipairs(os.files(path.join(source_basepath, pattern))) do
			file_list[path.relative(f, os.projectdir())] = true
		end
	end

	local dep_list = {}
	for f, _ in pairs(file_list) do
		table.insert(dep_list, f)
	end
	table.insert(dep_list, source_path)

	-- Pack source file and compile

	depend.on_changed(function ()
		progress.show(opt.progress, "${color.build.object}packing.$(mode) %s", source_path)

		os.vrunv(python.program, {
			paths.script,
			"--input", source_path, 
			"--output", cpp_output_path, 
			"--type", "source", 
			"--namespace", namespace, 
			"--varname", varname,
			"--root", source_basepath
		}, {curdir = os.projectdir()})

		compiler.compile(cpp_output_path, object_output_path, {target = target})

	end, {
		files = header_output_path, 
		dependfile = target:dependfile(header_output_path),
		lastmtime = os.mtime(object_output_path),
		changed = target:is_rebuilt() or not os.exists(cpp_output_path) or not os.exists(object_output_path)
	})

	table.insert(target:objectfiles(), object_output_path)
end

-- Pack asset files into C++ source/header files
rule("asset.pack")
	set_extensions(".pack-desc")
	on_load(_load)
	on_prepare_file(_prepare)
	on_build_file(_build)