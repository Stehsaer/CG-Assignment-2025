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
		python = find_tool("python3") or find_tool("python"),
		glslc = find_tool("glslc")
	}

	assert(tools.python, "Python not found")
	assert(tools.glslc, "GLSL compiler not found")

	-- Compute paths

	local source_name = path.filename(source_path) 								
	local header_output_path = path.join(paths.header, source_name .. ".hpp") 	
	local includedir = target:extraconf("rules", "asset.shader", "includedir") or nil

	local namespace = "shader_asset"
	local varname = string.gsub(source_name, "[%.%-]", "_")

	-- Find directories

	local function parse_dependencies(line)
		local deps = {}

		local colon_pos = string.find(line, ":")
		if not colon_pos then
			return deps
		end

		local deps_str = string.sub(line, colon_pos + 1)

		deps_str = deps_str
			:gsub(".*?:", "") 
			:match("^%s*(.-)%s*$") or ""

		for dep in deps_str:gmatch('"(.+)"') do
			dep = dep:gsub('^"(.*)"$', '%1')
			if dep ~= "" then
				table.insert(deps, dep)
			end
		end

		for dep in deps_str:gmatch("([^%s]+)") do
			dep = dep:gsub('^"(.*)"$', '%1')
			if dep ~= "" and dep:find("\"") == nil then
				table.insert(deps, dep)
			end
		end

		return deps
	end

	local stdout, _ = os.iorunv(tools.glslc.program, {"-M", source_path})
	local dependencies = parse_dependencies(stdout)
	
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
		files = table.join(dependencies, {header_output_path}),
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
		glslc = find_tool("glslc"),
		glslangValidator = find_tool("glslangValidator"),
		python = find_tool("python3") or find_tool("python")
	}

	assert(tools.glslc, "GLSL compiler not found")
	assert(tools.glslangValidator, "glslangValidator not found")
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
	local debug = target:extraconf("rules", "asset.shader", "debug") or false
	local includedir = target:extraconf("rules", "asset.shader", "includedir") or nil

	-- Compile shader to SPIR-V and generate C++ source file

	depend.on_changed(function() 
		progress.show(opt.progress, "${color.build.object}compiling.shader %s", source_path)

		-- Compile shader into SPIR-V

		if debug then
			os.vrunv(tools.glslangValidator.program, 
				{
					"--target-env", targetenv, 
					"-gVS",
					"--nan-clamp",
					includedir and format("-I%s", path.join(target:scriptdir(), includedir)) or "-I.",
					"-o", spv_temp_path, 
					source_path
				}
			)
		else
			os.vrunv(tools.glslc.program, 
				{
					"--target-env=" .. targetenv, 
					includedir and format("-I%s", path.join(target:scriptdir(), includedir)) or "-I.",
					"-o", spv_temp_path,
					"-O",
					"-fnan-clamp",
					source_path
				}
			)
		end

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
