#include "wavefront.hpp"
#include "build.hpp"

namespace wavefront
{
	std::expected<Object, util::Error> parse_string(const std::string_view& content) noexcept
	{
		return detail::parse_tokenize(content).and_then(detail::build_object);
	}

	std::expected<Object, util::Error> parse_raw(std::span<const std::byte> content) noexcept
	{
		static_assert(sizeof(char) == sizeof(std::byte));

		const auto string_view =
			std::string_view(reinterpret_cast<const char*>(content.data()), content.size());
		return parse_string(string_view);
	}

}
