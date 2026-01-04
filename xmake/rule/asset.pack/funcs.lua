import("core.base.binutils")
import("core.base.json")
import("core.project.config")
import("core.project.depend")
import("lib.detect.find_tool")
import("utils.progress")
import("utils.archive")
import("core.tool.compiler")

-- Get directories for generated files
function _get_path(target)
	local gen_header_root = path.join(target:autogendir(), "codegen-include")
	local gen_header_path = path.join(gen_header_root, "asset")
	local gen_temp_path = path.join(target:autogendir(), ".pack-temp", config.get("mode"))
	local script_path = path.join(os.projectdir(), "script", "gzip-script.py")

	return {
		header_root = gen_header_root,
		header = gen_header_path,
		archive = path.join(gen_temp_path, "archive"),
		cpp = path.join(gen_temp_path, "cpp"),
		script = script_path
	}
end

-- Create output directories
function _create_dir(paths)
	if not os.exists(paths.header) then
		os.mkdir(paths.header)
	end
	if not os.exists(paths.cpp) then
		os.mkdir(paths.cpp)
	end
end

-- Get output file paths
function _get_files(target, paths, source_path)
	local source_name = path.filename(source_path)
	local source_basepath = path.directory(source_path)

	local cpp_output_path = path.join(paths.cpp, source_name .. ".cpp")
	local object_output_path = target:objectfile(cpp_output_path)
	local header_output_path = path.join(paths.header, path.basename(source_path) .. ".hpp")

	local varname = string.gsub(path.basename(source_path), "[%.%-]", "_")

	return {
		source = source_path,
		base = source_basepath,
		cpp = cpp_output_path,
		object = object_output_path,
		header = header_output_path,
		varname = varname
	}
end

-- Parse and find file from json
-- Returns: list of files, absolute path
function _parse_filelist(json_path)
	local glob_list = json.loadfile(json_path)
	local base_path = path.directory(json_path)
	local file_table_bool = {}

	for _, pattern in ipairs(glob_list) do
		for _, f in ipairs(os.files(path.join(base_path, pattern))) do
			file_table_bool[f] = true
		end
	end

	local file_list = {}
	for f, _ in pairs(file_table_bool) do
		table.insert(file_list, f)
	end

	return file_list
end

-- Generate header file content
function _get_header(files)
	local file_template = [[
		#pragma once
		#include <span>
		#include <map>
		#include <string>
		#include <cstddef>

		namespace resource_asset
		{
			extern const std::map<std::string, std::span<const std::byte>> %s;
		}
	]]

	return format(file_template, files.varname)
end

-- Generate symbol name for a file
function _get_symbol_name(target_name, rel_file_path)
	local mangled_name = string.gsub(rel_file_path, "[%.%-/\\\\]", "_")
	return format("_asset_pack_%s_%s", target_name, mangled_name)
end

-- Generate cpp file content
function _get_cpp(target, files, file_list)
	local file_template = [[
		#include <span>
		#include <map>
		#include <string>
		#include <cstddef>

		extern "C" 
		{
			%s
		}

		namespace resource_asset
		{
			extern const std::map<std::string, std::span<const std::byte>> %s = {
				%s
			};
		}
	]]

	local target_name = target:name()

	local extern_decl = ""
	local map_entries = ""

	for _, file in ipairs(file_list) do
		local relative_path = path.relative(file, files.base)
		local symbol_name = _get_symbol_name(target_name, relative_path)

		local file_extern_decl = format(
			"extern const std::byte %s_start; extern const std::byte %s_end;",
			symbol_name,
			symbol_name
		)

		local file_map_entry = format(
			"{\"%s\", {&%s_start, &%s_end}},",
			relative_path,
			symbol_name,
			symbol_name
		)

		extern_decl = extern_decl .. file_extern_decl .. "\n"
		map_entries = map_entries .. file_map_entry .. "\n"
	end

	return format(file_template, extern_decl, files.varname, map_entries)
end

function load_rule(target)
	local paths = _get_path(target)
	local public_include = target:extraconf("rules", "asset.pack", "public_include") or false
	target:add("includedirs", paths.header_root, {public=public_include})
end

function prepare_file(target, source_path, opt)
	local paths = _get_path(target)
	local files = _get_files(target, paths, source_path)
	local file_list = _parse_filelist(source_path)

	_create_dir(paths)

	local header_content = _get_header(files)
	local cpp_content = _get_cpp(target, files, file_list)

	depend.on_changed(function ()
		io.writefile(files.header, header_content)
		io.writefile(files.cpp, cpp_content)
	end, {
		files = table.join({source_path}, file_list),
		dependfile = target:dependfile(files.source),
		lastmtime = os.mtime(files.header),
		changed = target:is_rebuilt() or not os.exists(files.header) or not os.exists(files.cpp)
	})
end

function build_file(target, source_path, opt)
	local paths = _get_path(target)
	local files = _get_files(target, paths, source_path)
	local file_list = _parse_filelist(source_path)
	local python = assert(find_tool("python3") or find_tool("python"), "python not found!")

	local bin2obj_files = {}
	local target_name = target:name()

	for _, file in ipairs(file_list) do
		local relative_path = path.relative(file, files.base)
		local symbol_name = _get_symbol_name(target_name, relative_path)
		local object_file = target:objectfile(file)

		table.insert(bin2obj_files, object_file)
	end

	depend.on_changed(function ()

		for _, file in ipairs(file_list) do
			local relative_path = path.relative(file, files.base)
			local symbol_name = _get_symbol_name(target_name, relative_path)
			local object_file = target:objectfile(file)
			local archive_file = path.join(paths.archive, path.filename(file) .. ".gz")

			os.vrunv(python.program, {paths.script, file, archive_file})
			binutils.bin2obj(archive_file, object_file, {symbol_prefix = "", basename = symbol_name})
		end

		compiler.compile(files.cpp, files.object, {target = target})
	end, {
		files = {files.header, files.cpp},
		dependfile = target:dependfile(files.header),
		lastmtime = os.mtime(files.object),
		changed = target:is_rebuilt() or not os.exists(files.object)
	})

	table.insert(target:objectfiles(), files.object)
	for _, obj in ipairs(bin2obj_files) do
		table.insert(target:objectfiles(), obj)
	end
end