#include "parse-line.hpp"
#include "helper.hpp"

#include <map>
#include <print>

namespace wavefront::detail
{
	std::expected<ParsedLine, util::Error> parse_pos(const std::vector<std::string>& parameters) noexcept
	{
		if (parameters.size() != 4) return util::Error("Invalid arguments");

		const auto x = to_number<float>(parameters[1]);
		const auto y = to_number<float>(parameters[2]);
		const auto z = to_number<float>(parameters[3]);

		if (!x || !y || !z) return util::Error("Invalid coordinates");

		return PositionLine{
			.pos = {*x, *y, *z}
		};
	}

	std::expected<ParsedLine, util::Error> parse_uv(const std::vector<std::string>& parameters) noexcept
	{
		if (parameters.size() != 3) return util::Error("Invalid arguments");

		const auto u = to_number<float>(parameters[1]);
		const auto v = to_number<float>(parameters[2]);

		if (!u || !v) return util::Error("Invalid coordinates");

		return UvLine{
			.uv = {*u, *v}
		};
	}

	std::expected<ParsedLine, util::Error> parse_normal(const std::vector<std::string>& parameters) noexcept
	{
		if (parameters.size() != 4) return util::Error("Invalid arguments");

		const auto x = to_number<float>(parameters[1]);
		const auto y = to_number<float>(parameters[2]);
		const auto z = to_number<float>(parameters[3]);

		if (!x && !y && !z) return util::Error("Invalid coordinates");

		return NormalLine{
			.normal = {*x, *y, *z}
		};
	}

	std::expected<ParsedLine, util::Error> parse_face(const std::vector<std::string>& parameters) noexcept
	{
		if (parameters.size() != 4) util::Error("Invalid arguments");

		const auto v1 = parse_face_index(parameters[1]);
		const auto v2 = parse_face_index(parameters[2]);
		const auto v3 = parse_face_index(parameters[3]);

		if (!v1 || !v2 || !v3) return util::Error("Invalid vertex index");

		return FaceLine{.v1 = *v1, .v2 = *v2, .v3 = *v3};
	}

	std::expected<ParsedLine, util::Error> parse_line(const std::string_view& slice) noexcept
	{
		/* Slice */

		if (slice.empty()) return std::monostate();
		if (slice.length() > 4096) return util::Error("Line input too long, should not exceed 4096");

		const auto parameters =
			slice
			| std::views::filter([](char c) { return c != '\r'; })
			| std::views::split(' ')
			| std::views::filter([](auto subrange) { return !subrange.empty(); })
			| std::views::transform([](auto subrange) {
				  return std::string(subrange.begin(), subrange.end());
			  })
			| std::ranges::to<std::vector>();

		if (parameters.empty()) return std::monostate();

		/* Parse */

		using Parse_func = std::expected<ParsedLine, util::Error>(const std::vector<std::string>&);

		const std::map<std::string, Parse_func*> parsers = {
			{"v",  parse_pos   },
			{"vt", parse_uv    },
			{"vn", parse_normal},
			{"f",  parse_face  }
		};

		// std::println("Parsing line: {}", parameters);

		if (const auto it = parsers.find(std::string(parameters[0])); it != parsers.end())
			return it->second(parameters);
		else
			return std::monostate();
	}
}