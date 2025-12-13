function _load(target)
	import("get_path")
	local paths = get_path(target)
	local public_include = target:extraconf("rules", "asset.shader", "public_include") or false
	target:add("includedirs", paths.header_root, {public=public_include})
end

function _prepare(target, source_path, opt)

	import("lib.detect.find_tool")
	import("get_path")
	import("core.base.json")
	import("core.project.depend")
	import("utils.progress")

	local paths = get_path(target)

	-- Create output directories

	if not os.exists(paths.header) then
		os.mkdir(paths.header)
	end
	if not os.exists(paths.temp) then
		os.mkdir(paths.temp)
	end

	-- Get tools

	local paths = get_path(target)

	local tools = {
		python = find_tool("python3") or find_tool("python")
	}

	assert(tools.python, "Python not found")

	-- Find directories

	local source_name = path.filename(source_path) 								
	local header_output_path = path.join(paths.header, source_name .. ".hpp") 	
	local includedir = target:extraconf("rules", "asset.shader", "includedir") or nil

	local namespace = "shader_asset"
	local varname = string.gsub(source_name, "[%.%-]", "_")

	local depend_targets = {source_path}
	local includedir_files = includedir and os.files(path.join(target:scriptdir(), includedir, "**")) or {}
	for _, f in ipairs(includedir_files) do
		table.insert(depend_targets, f)
	end

	-- Generate header file

	depend.on_changed(function ()
		os.vrunv(tools.python.program, {
			paths.script,
			"--input", source_path, 
			"--output", header_output_path, 
			"--type", "header", 
			"--namespace", namespace, 
			"--varname", varname
		})
	end,{
		files = depend_targets,
		dependfile = target:dependfile(source_path),
		lastmtime = os.mtime(header_output_path),
		changed = target:is_rebuilt() or not os.exists(header_output_path)
	})
end

function _build(target, source_path, opt)
	
	import("lib.detect.find_tool")
	import("core.project.config")
	import("core.project.depend")
	import("core.tool.compiler")
	import("utils.progress")
	import("get_path")

	-- Get tools

	local paths = get_path(target)

	local tools = {
		glsl_compiler = find_tool("glslc"),
		python = find_tool("python3") or find_tool("python")
	}

	assert(tools.glsl_compiler, "GLSL compiler not found")
	assert(tools.python, "Python not found")

	-- Find directories

	local source_name = path.filename(source_path) 								
	local spv_temp_path = path.join(paths.temp, source_name .. ".spv") 			
	local unopt_spv_temp_path = path.join(paths.temp, source_name .. ".unopt.spv")
	local cpp_temp_path = path.join(paths.temp, source_name .. ".cpp") 			
	local object_output_path = target:objectfile(cpp_temp_path) 				
	local header_output_path = path.join(paths.header, source_name .. ".hpp") 	

	local namespace = "shader_asset"
	local varname = string.gsub(source_name, "[%.%-]", "_")

	local targetenv = target:extraconf("rules", "asset.shader", "targetenv") or "vulkan1.3"
	local includedir = target:extraconf("rules", "asset.shader", "includedir") or nil

	-- Compile shader to SPIR-V and generate C++ source file

	depend.on_changed(function() 
		progress.show(opt.progress, "${color.build.object}compiling.shader %s", source_path)

		-- Compile shader into SPIR-V

		os.vrunv(tools.glsl_compiler.program, 
			{
				"--target-env=" .. targetenv, 
				"-g", 
				includedir and format("-I%s", path.join(target:scriptdir(), includedir)) or "-I.",
				"-o", spv_temp_path,
				"-O",
				source_path
			}
		)

		-- Generate C++ source file

		os.vrunv(tools.python.program, 
			{
				paths.script, 
				"--input", spv_temp_path, 
				"--output", cpp_temp_path, 
				"--type", "source", 
				"--namespace", namespace, 
				"--varname", varname
			}
		)

		-- Compile C++ source file

		compiler.compile(cpp_temp_path, object_output_path, {target=target})

	end, {
		files = header_output_path,
		dependfile = target:dependfile(header_output_path),
		lastmtime = os.mtime(object_output_path),
		changed = target:is_rebuilt() or not os.exists(cpp_temp_path) or not os.exists(object_output_path)
	})

	table.insert(target:objectfiles(), object_output_path)
end

-- Compile shaders and pack as C++ files
rule("asset.shader")
	set_extensions(".vert", ".frag", ".comp")
	on_load(_load)
	on_prepare_file(_prepare)
	on_build_file(_build)
