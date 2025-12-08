#include "util/asset.hpp"

namespace util
{
	std::expected<std::span<const std::byte>, util::Error> get_asset(
		const std::map<std::string, std::span<const std::byte>>& data,
		const std::string& name
	) noexcept
	{
		const auto find = data.find(name);
		if (find == data.end()) return util::Error(std::format("未找到资源: {}", name));
		return find->second;
	}
}