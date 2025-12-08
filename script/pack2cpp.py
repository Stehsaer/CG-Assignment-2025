import gzip
import argparse
import json
import base64
import multiprocessing
import glob
import os


def _get_namespace_string(namespace: str) -> tuple[str, str]:
    namespace_split = namespace.split("::")
    namespace_heading = "".join([f"namespace {ns} {{ " for ns in namespace_split])
    namespace_trailing = "}" * len(namespace_split)
    return namespace_heading, namespace_trailing


def _get_header(namespace: str, varname: str) -> list[str]:
    heading, trailing = _get_namespace_string(namespace)
    return [
        "// Auto-generated file.",
        "#pragma once",
        "#include <map>",
        "#include <string>",
        "#include <span>",
        "#include <cstddef>",
        heading,
        f"	  extern const std::map<std::string, std::span<const std::byte>> {varname};",
        trailing,
    ]


def _compress_file(path: str) -> bytes:
    with open(path, "rb") as f:
        data: bytes = f.read()
    return gzip.compress(data)


def _get_file_list(root: str, path: str) -> list[tuple[str, str]]:
    with open(path, "r") as f:
        data = json.load(f)

    if not isinstance(data, list):
        raise ValueError("Invalid JSON format: expected a list")

    for item in data:
        if not isinstance(item, str):
            raise ValueError("Invalid JSON format: expected a list of paths in string")

    file_set = set(
        [
            (file, os.path.relpath(file, root))
            for pattern in data
            for file in glob.glob(os.path.join(root, pattern), recursive=True)
            if os.path.isfile(file)
        ]
    )
    file_list = list(file_set)
    file_list.sort()

    return file_list


def _encode_path(path: str) -> str:
    base32_encoded: str = "d" + base64.b32encode(path.encode("utf-8")).decode(
        "ascii"
    ).replace("=", "_")

    return base32_encoded


def _to_cpp_array_line(path: tuple[str, str]) -> str:
    absolute, relative = path
    data = _compress_file(absolute)
    size = len(data)
    data_str = ",".join([f"0x{byte:02x}" for byte in data])
    varname = _encode_path(relative)
    return f"    static const std::array<uint8_t, {size}> {varname} = {{ {data_str} }};"


def _to_cpp_map_entry(path: tuple[str, str]) -> str:
    _, relative = path
    linux_relative = relative.replace("\\", "/")
    varname = _encode_path(relative)
    return f'{{R"({linux_relative})", std::as_bytes(std::span({varname}))}}'


def _get_args():
    parser = argparse.ArgumentParser(
        description="Compress and pack multiple files into C++ source code."
    )

    parser.add_argument(
        "--input",
        help="Input JSON Description",
        required=True,
        type=str,
        dest="input",
    )

    parser.add_argument(
        "--output", help="Output Path", required=True, type=str, dest="output"
    )

    parser.add_argument(
        "--type",
        help="Output type: header/source",
        required=True,
        type=str,
        dest="type",
    )

    parser.add_argument(
        "--namespace",
        help="C++ Namespace",
        required=True,
        type=str,
        dest="namespace",
    )

    parser.add_argument(
        "--varname",
        help="C++ Variable Name",
        required=True,
        type=str,
        dest="varname",
    )

    parser.add_argument(
        "--root",
        help="Path Prefix for input path",
        required=False,
        type=str,
        dest="root",
        default=".",
    )

    return parser.parse_args()


def main():
    args = _get_args()

    if args.type == "header":
        header_str = "\n".join(_get_header(args.namespace, args.varname)) + "\n"
        with open(args.output, "w") as f:
            f.write(header_str)
        return

    if args.type == "source":
        heading, trailing = _get_namespace_string(args.namespace)

        file_list = _get_file_list(args.root, args.input)

        if len(file_list) == 0:
            cpp_array_lines = []
        else:
            cpp_array_lines = multiprocessing.Pool(
                min(multiprocessing.cpu_count(), len(file_list))
            ).map(_to_cpp_array_line, file_list)

        prefix_lines = [
            "// Auto-generated file, do not modify",
            "#include <array>",
            "#include <map>",
            "#include <string>",
            "#include <span>",
            "#include <cstdint>",
            "#include <cstddef>",
            heading,
        ]
        suffix_lines = [trailing, ""]

        map_entries = ",".join([_to_cpp_map_entry(path) for path in file_list])
        map_str = f"    extern const std::map<std::string, std::span<const std::byte>> {args.varname} = {{ {map_entries} }};"

        result_string = "\n".join(
            prefix_lines + cpp_array_lines + [map_str] + suffix_lines
        )

        with open(args.output, "w") as f:
            f.write(result_string)
        return

    raise ValueError("Unknown type")


if __name__ == "__main__":
    main()
