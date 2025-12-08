#include "util/unwrap.hpp"

namespace util
{
	Unwrap unwrap(const std::string& message, const std::source_location& location) noexcept
	{
		return Unwrap{.location = location, .message = message};
	}

	void operator|(std::expected<void, Error> expected, const Unwrap& unwrap)
	{
		if (!expected) throw expected.error().propagate(unwrap.message, unwrap.location);
	}
}