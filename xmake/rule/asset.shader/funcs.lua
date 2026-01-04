import("lib.detect.find_tool")
import("core.project.depend")
import("utils.progress")
import("core.project.config")
import("core.project.depend")
import("core.base.binutils")

function _get_path(target)
	local gen_header_root_path = path.join(target:autogendir(), "codegen-include")
	local gen_header_path = path.join(gen_header_root_path, "asset", "shader")
	local gen_temp_path = path.join(target:autogendir(),".shader-temp", config.get("mode"), "gen")
	local script_path = path.join(os.projectdir(), "script", "spv2cpp.py")

	return {
		header_root = gen_header_root_path,
		header = gen_header_path,
		temp = gen_temp_path,
		script = script_path
	}
end

function _create_dir(paths)
	if not os.exists(paths.header) then
		os.mkdir(paths.header)
	end
	if not os.exists(paths.temp) then
		os.mkdir(paths.temp)
	end
end

function _get_tools()
	local tools = {
		glslc = find_tool("glslc"),
		glslangValidator = find_tool("glslangValidator")
	}

	assert(tools.glslc, "GLSL compiler not found")
	assert(tools.glslangValidator, "glslangValidator not found")

	return tools
end

function _get_filepaths(target, paths, source_path)
	local source_name = path.filename(source_path)				
	local spv_temp_path = path.join(paths.temp, source_name .. ".spv") 			
	local header_output_path = path.join(paths.header, source_name .. ".hpp")
	local object_output_path = target:objectfile(source_name)
	local varname = string.gsub(source_name, "[%.%-]", "_")

	return {
		source = source_path,
		spv = spv_temp_path,
		header = header_output_path,
		object = object_output_path,
		varname = varname
	}
end

function _parse_dependencies(line)
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

function _compile_spv(tools, files, debug)
	local target_env = "vulkan1.2"

	if debug then
		os.vrunv(tools.glslangValidator.program, 
			{
				"--target-env", target_env,
				"-gVS",
				"--nan-clamp",
				"-o", files.spv, 
				files.source
			}
		)
	else
		os.vrunv(tools.glslc.program, 
			{
				"--target-env=" .. target_env, 
				"-o", files.spv,
				"-O",
				"-fnan-clamp",
				files.source
			}
		)
	end
end

function load_rule(target)
	local paths = _get_path(target)
	local public_include = target:extraconf("rules", "asset.shader", "public_include") or false
	target:add("includedirs", paths.header_root, {public=public_include})
end

function prepare_file(target, source_path, opt)

	local paths = _get_path(target)
	_create_dir(paths)

	local tools = _get_tools()
	local files = _get_filepaths(target, paths, source_path)

	local stdout, _ = os.iorunv(tools.glslc.program, {"-M", source_path})
	local dependencies = _parse_dependencies(stdout)
	
	local target_name = string.gsub(target:name(), "[%.%-]", "_")
	local symbol_name = format("_asset_shader_%s_%s", target_name, files.varname)

	local header_file_template = [[
		#pragma once
		#include <span>

		extern "C" 
		{
			extern const std::byte %s_start;
			extern const std::byte %s_end;
		}

		namespace shader_asset
		{
			inline static const std::span<const std::byte> %s = {
				&%s_start,
				&%s_end
			};
		}
	]];

	local header_file_content = format(
		header_file_template, 
		symbol_name, 
		symbol_name, 
		files.varname, 
		symbol_name, 
		symbol_name
	);

	depend.on_changed(function ()
		io.writefile(files.header, header_file_content)
	end,{
		files = table.join(dependencies, {files.source}),
		dependfile = target:dependfile(files.source),
		lastmtime = os.mtime(files.header),
		changed = target:is_rebuilt() or not os.exists(files.header)
	})
end

function build_file(target, source_path, opt)
	
	local paths = _get_path(target)
	local tools = _get_tools()
	local files = _get_filepaths(target, paths, source_path)

	local debug = target:extraconf("rules", "asset.shader", "debug") or false

	depend.on_changed(function() 
		progress.show(opt.progress, "${color.build.object}compiling.shader %s", source_path)
		_compile_spv(tools, files, debug)
		binutils.bin2obj(files.spv, files.object, {
			symbol_prefix = format("_asset_shader_%s_", string.gsub(target:name(), "[%.%-]", "_")),
			basename = files.varname
		})
	end, {
		files = files.header,
		dependfile = target:dependfile(files.header),
		lastmtime = os.mtime(files.object),
		changed = target:is_rebuilt() or not os.exists(files.object)
	})

	table.insert(target:objectfiles(), files.object)
end
