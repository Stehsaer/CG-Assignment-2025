#include "build.hpp"
#include "helper.hpp"
#include "parse-line.hpp"

#include <algorithm>
#include <ranges>

namespace wavefront::detail
{
	static bool is_string_empty(const std::string& str) noexcept
	{
		return str.empty() || std::ranges::all_of(str, isspace);
	}

	std::expected<std::vector<ParsedLine>, util::Error> parse_tokenize(
		const std::string_view& content
	) noexcept
	{
		std::vector<std::string> lines;
		for (const auto& line_range : content | std::views::split('\n'))
		{
			std::string line(std::from_range, line_range);
			if (!is_string_empty(line)) lines.push_back(std::move(line));
		}

		std::vector<ParsedLine> result;
		for (const auto [i, line] : lines | std::views::enumerate)
		{
			const auto parsed_line = parse_line(line);
			if (!parsed_line)
				return parsed_line.error().forward(std::format("Parsing failed at line {}", i + 1));
			if (!std::holds_alternative<std::monostate>(*parsed_line)) result.push_back(*parsed_line);
		}

		return result;
	}

	std::expected<Object, util::Error> build_object(const std::vector<ParsedLine>& lines) noexcept
	{
		/* Extract */

		const auto positions = extract_lines<PositionLine>(lines);
		const auto uvs = extract_lines<UvLine>(lines);
		const auto normals = extract_lines<NormalLine>(lines);

		const auto vertex_indices = extract_lines<FaceLine>(lines)
			| std::views::transform(&FaceLine::as_array)
			| std::views::join
			| std::ranges::to<std::vector>();

		std::vector<Vertex> vertices;
		vertices.reserve(vertex_indices.size());

		for (const auto& [pos, uv, normal] : vertex_indices)
		{
			if (pos > positions.size() || uv > uvs.size() || normal > normals.size())
				return util::Error("Face index out of bounds");

			vertices.emplace_back(
				Vertex{.pos = positions[pos].pos, .normal = normals[normal].normal, .uv = uvs[uv].uv}
			);
		}

		return Object{.vertices = std::move(vertices)};
	}
}