#pragma once

#include <functional>
#include <map>
#include <optional>

namespace util
{
	template <
		typename K,
		typename V,
		typename Q,
		typename Pred = std::less<K>,
		typename Alloc = std::allocator<std::pair<K, V>>
	>
	std::optional<std::reference_wrapper<const V>> find_map(
		const std::map<K, V, Pred, Alloc>& m,
		const Q& key
	) noexcept
	{
		const auto it = m.find(K(key));
		if (it == m.end()) return std::nullopt;
		return std::cref(it->second);
	}

	template <
		typename K,
		typename V,
		typename Q,
		typename Pred = std::less<K>,
		typename Alloc = std::allocator<std::pair<K, V>>
	>
	std::optional<std::reference_wrapper<V>> find_map(std::map<K, V, Pred, Alloc>& m, const K& key) noexcept
	{
		const auto it = m.find(K(key));
		if (it == m.end()) return std::nullopt;
		return std::ref(it->second);
	}
}