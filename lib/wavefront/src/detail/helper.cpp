#include "helper.hpp"

#include <memory_resource>

namespace wavefront::detail
{
	std::optional<FaceLine::Index> parse_face_index(const std::string_view& slice) noexcept
	{
		std::array<std::string_view, 16> buffer;
		std::pmr::monotonic_buffer_resource pool(buffer.data(), buffer.size());

		const auto parameters =
			slice
			| std::views::split('/')
			| std::views::transform([](auto subrange) {
				  return std::string_view(subrange.begin(), subrange.end());
			  })
			| std::ranges::to<std::pmr::vector<std::string_view>>(&pool);

		if (parameters.empty() || parameters.size() != 3) return std::nullopt;

		const auto pos_index = to_number<uint32_t>(parameters[0]);
		const auto uv_index = to_number<uint32_t>(parameters[1]);
		const auto normal_index = to_number<uint32_t>(parameters[2]);

		if (!pos_index || !uv_index || !normal_index) return std::nullopt;
		if (*pos_index == 0 || *uv_index == 0 || *normal_index == 0) return std::nullopt;

		return FaceLine::Index{
			.pos_index = *pos_index - 1,
			.uv_index = *uv_index - 1,
			.normal_index = *normal_index - 1
		};
	}
}