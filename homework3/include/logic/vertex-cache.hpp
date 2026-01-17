#pragma once

#include "geometry/primitive.hpp"
#include "geometry/vertex.hpp"

#include <optional>
#include <span>

namespace logic
{
	template <primitive::PrimitiveType T>
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