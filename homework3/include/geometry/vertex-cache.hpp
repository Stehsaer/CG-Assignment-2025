#pragma once

#include "vertex.hpp"
#include <optional>
#include <span>
namespace primitive
{
	template <typename T>
	concept PrimitiveType = requires(T a) {
		{ a.gen_vertices() } -> std::same_as<std::vector<LineVertex>>;
	} && std::copyable<T> && std::equality_comparable<T>;

	template <PrimitiveType T>
	class VertexCache
	{
		struct CacheEntry
		{
			T primitive;
			std::vector<LineVertex> vertices;
		};

		std::optional<CacheEntry> cached_vertices;

	  public:

		bool update(const T& primitive) noexcept
		{
			if (cached_vertices.has_value() && cached_vertices->primitive == primitive) return false;

			cached_vertices = CacheEntry{.primitive = primitive, .vertices = primitive.gen_vertices()};
			return true;
		}

		std::span<const LineVertex> get() const noexcept
		{
			if (!cached_vertices.has_value()) return {};
			return cached_vertices->vertices;
		}
	};
}