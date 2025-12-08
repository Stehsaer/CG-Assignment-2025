#include "line-elem.hpp"

#include <tuple>

namespace wavefront::detail
{
	std::strong_ordering Face_line::Index::operator<=>(const Index& other) const noexcept
	{
		return std::tie(pos_index, uv_index, normal_index)
			<=> std::tie(other.pos_index, other.uv_index, other.normal_index);
	}

	std::array<Face_line::Index, 3> Face_line::as_array() const noexcept
	{
		return {v1, v2, v3};
	}
}